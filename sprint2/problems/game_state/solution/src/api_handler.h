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
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            sendMethodNotAllowed(req, send, "GET, HEAD");
            return;
        }

        auto auth_header_it = req.find(boost::beast::http::field::authorization);
        if (auth_header_it == req.end() || auth_header_it->value().empty()) {
            sendUnauthorized(req, send, "invalidToken", "Authorization header is missing");
            return;
        }

        std::string token = std::string(auth_header_it->value());
        const std::string prefix = "Bearer ";
        if (token.substr(0, prefix.size()) != prefix) {
            sendUnauthorized(req, send, "invalidToken", "Authorization header is invalid");
            return;
        }
        token = token.substr(prefix.size());

        model::Player* player = nullptr;
        for (auto& map : game_.GetMaps()) {
            for (auto* p : players_.GetPlayersByMap(map.GetId())) {
                if (p->GetToken() == token) {
                    player = p;
                    break;
                }
            }
            if (player) break;
        }

        if (!player) {
            sendUnauthorized(req, send, "unknownToken", "Player token has not been found");
            return;
        }

        const model::Map::Id map_id = player->GetMapId();
        json::object resp;

        for (auto* p : players_.GetPlayersByMap(map_id)) {
            resp[std::to_string(p->GetId())] = {{"name", p->GetName()}};
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        if (req.method() != http::verb::head) {
            res.body() = json::serialize(resp);
        }
        res.prepare_payload();
        send(std::move(res));
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleState(http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            return Send405(send, req);
        }

        auto it = req.find(http::field::authorization);
        if (it == req.end()) {
            return Send401(send, "invalidToken", "Authorization header is required");
        }

        std::string auth = std::string(it->value());

        if (!auth.starts_with("Bearer ")) {
            return Send401(send, "invalidToken", "Authorization header is invalid");
        }

        std::string token = auth.substr(7);

        auto player = players_.FindByToken(token);
        if (!player) {
            return Send401(send, "unknownToken", "Player token has not been found");
        }

        json::object players_json;

        auto map_id = player->GetMapId();

        for (auto* p : players_.GetPlayersByMap(map_id)) {
            json::object obj;

            obj["pos"] = {p->GetPosition().x, p->GetPosition().y};
            obj["speed"] = {p->GetSpeed().dx, p->GetSpeed().dy};
            obj["dir"] = "U";

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
    }

private:
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