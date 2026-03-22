#pragma once

#include "model.h"
#include <boost/json.hpp>
#include "http_server.h"
#include "player.h"
#include <string>

namespace http_handler {

namespace http = boost::beast::http;
namespace json = boost::json;

class ApiHandler {
public:
    ApiHandler(model::Game& game, model::PlayerManager& players)
        : game_(game), players_(players) {}

template <typename Send>
    void HandleJoin(const http::request<http::string_body>& req, Send&& send) {
        if (req.method() != http::verb::post) {
            sendMethodNotAllowed(req, send, "POST");
            return;
        }

        json::value body;
        try {
            body = json::parse(req.body());
        } catch (...) {
            sendBadRequest(req, send, "Join game request parse error");
            return;
        }

        auto obj = body.as_object();
        if (!obj.contains("userName") || !obj.contains("mapId")) {
            sendBadRequest(req, send, "Missing userName or mapId");
            return;
        }

        std::string name = obj["userName"].as_string().c_str();
        std::string map_id = obj["mapId"].as_string().c_str();

        if (name.empty()) {
            sendBadRequest(req, send, "Invalid name");
            return;
        }

        const model::Map* map = game_.FindMap(model::Map::Id{map_id});
        if (!map) {
            sendNotFound(req, send, "mapNotFound", "Map not found");
            return;
        }

        auto& player = players_.AddPlayer(name, map->GetId());

        json::object resp{
            {"authToken", player.GetToken()},
            {"playerId", player.GetId()}
        };

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.body() = json::serialize(resp);
        res.prepare_payload();
        send(std::move(res));
    }

    template <typename Send>
    void HandlePlayers(const http::request<http::string_body>& req, Send&& send) {
        ExecuteAuthorized(req, send, [&](model::Player* player) {
            json::object players_json;
            for (auto* p : players_.GetPlayersByMap(player->GetMapId())) {
                players_json[std::to_string(p->GetId())] = {{"name", p->GetName()}};
            }

            json::object result;
            result["players"] = players_json;

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            if (req.method() != http::verb::head) {
                res.body() = json::serialize(result);
            }
            res.prepare_payload();
            send(std::move(res));
        });
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleState(http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
        ExecuteAuthorized(req, send, [&](model::Player* player) {
            json::object players_json;

            for (auto* p : players_.GetPlayersByMap(player->GetMapId())) {
                json::object obj;
                obj["pos"] = {p->GetPosition().x, p->GetPosition().y};
                obj["speed"] = {p->GetSpeed().vx, p->GetSpeed().vy};
                obj["dir"] = "U";  // направление по умолчанию
                players_json[std::to_string(p->GetId())] = obj;
            }

            json::object result;
            result["players"] = players_json;

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            if (req.method() != http::verb::head) {
                res.body() = json::serialize(result);
            }
            res.prepare_payload();
            send(std::move(res));
        });
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleAction(http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
        if (req.method() != http::verb::post) {
            sendMethodNotAllowed(req, send, "POST");
            return;
        }

        if (req[http::field::content_type] != "application/json") {
            sendBadRequest(req, send, "Invalid content type");
            return;
        }

        auto ExecuteAuthorized = [&](auto&& action) {
        auto it = req.find(http::field::authorization);
        if (it == req.end()) {
            sendUnauthorized(req, send, "invalidToken", "Authorization header is required");
            return;
        }

        std::string auth = std::string(it->value());
        if (!auth.starts_with("Bearer ")) {
            sendUnauthorized(req, send, "invalidToken", "Authorization header is invalid");
            return;
        }

        std::string token = auth.substr(7);
        auto player = players_.FindByToken(token);
        if (!player) {
            sendUnauthorized(req, send, "unknownToken", "Player token has not been found");
            return;
        }

        action(player);
        };

        ExecuteAuthorized([&](model::Player* player) {
            json::value body;
            try {
                body = json::parse(req.body());
            } catch (...) {
                sendBadRequest(req, send, "Failed to parse action");
                return;
            }

            if (!body.as_object().contains("move")) {
                sendBadRequest(req, send, "Failed to parse action");
                return;
            }

            std::string move = body.at("move").as_string().c_str();
            if (move != "L" && move != "R" && move != "U" && move != "D" && move != "") {
                sendBadRequest(req, send, "Failed to parse action");
                return;
            }

            const model::Map* map = game_.FindMap(player->GetMapId());
            double s = map->GetDogSpeed();

            if (move == "L") player->SetSpeed({-s, 0}), player->SetDirection(model::Direction::WEST);
            else if (move == "R") player->SetSpeed({s, 0}), player->SetDirection(model::Direction::EAST);
            else if (move == "U") player->SetSpeed({0, -s}), player->SetDirection(model::Direction::NORTH);
            else if (move == "D") player->SetSpeed({0, s}), player->SetDirection(model::Direction::SOUTH);
            else player->SetSpeed({0, 0});

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = "{}";
            res.prepare_payload();
            send(std::move(res));
        });
    }

private:
    template <typename Body, typename Allocator, typename Send, typename Fn>
    void ExecuteAuthorized(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send, Fn&& action) {
        auto it = req.find(http::field::authorization);
        if (it == req.end() || it->value().empty()) {
            sendUnauthorized(req, send, "invalidToken", "Authorization header missing");
            return;
        }

        std::string auth = std::string(it->value());
        const std::string prefix = "Bearer ";
        if (!auth.starts_with(prefix)) {
            sendUnauthorized(req, send, "invalidToken", "Authorization header invalid");
            return;
        }

        std::string token = auth.substr(prefix.size());

        auto isValidToken = [](const std::string& t) {
            if (t.size() != 32) return false;
            for (char c : t) if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
            return true;
        };

        if (!isValidToken(token)) {
            sendUnauthorized(req, send, "invalidToken", "Token invalid");
            return;
        }

        auto player = players_.FindByToken(token);
        if (!player) {
            sendUnauthorized(req, send, "unknownToken", "Token not found");
            return;
        }

        action(player);
    }

    template <typename Send>
    void sendBadRequest(const http::request<http::string_body>& req, Send&& send, const std::string& message) {
        sendError(req, send, http::status::bad_request, "invalidArgument", message);
    }

    template <typename Send>
    void sendNotFound(const http::request<http::string_body>& req, Send&& send, const std::string& code, const std::string& message) {
        sendError(req, send, http::status::not_found, code, message);
    }

    template <typename Send>
    void sendUnauthorized(const http::request<http::string_body>& req, Send&& send, const std::string& code, const std::string& message) {
        sendError(req, send, http::status::unauthorized, code, message);
    }

    template <typename Send>
    void sendMethodNotAllowed(const http::request<http::string_body>& req, Send&& send, const std::string& allow) {
        http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
        res.set(http::field::allow, allow);
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");

        json::object body{
            {"code", "invalidMethod"},
            {"message", "Invalid method"}
        };
        res.body() = json::serialize(body);
        res.prepare_payload();
        send(std::move(res));
    }

    template <typename Send>
    void sendError(const http::request<http::string_body>& req, Send&& send, http::status status,
                   const std::string& code, const std::string& message) {
        http::response<http::string_body> res{status, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");

        json::object body{
            {"code", code},
            {"message", message}
        };
        res.body() = json::serialize(body);
        res.prepare_payload();
        send(std::move(res));
    }

private:
    model::Game& game_;
    model::PlayerManager& players_;
};

} // namespace http_handler