#pragma once

#include <string>
#include <unordered_map>
#include <boost/beast/http.hpp>
#include "model.h"
#include "json.h"

namespace http_handler {

namespace http = boost::beast::http;
using json::json;

class ApiHandler {
public:
    ApiHandler(model::Game& game, model::PlayerManager& players)
        : game_(game), players_(players) {}

    template <typename Send>
    void HandleJoin(const http::request<http::string_body>& req, Send&& send) {
        if (req.method() != http::verb::post) {
            sendMethodNotAllowed(req, std::forward<Send>(send), "POST");
            return;
        }

        json::value body;
        try {
            body = json::parse(req.body());
        } catch (...) {
            sendBadRequest(req, std::forward<Send>(send), "Join game request parse error");
            return;
        }

        auto obj = body.as_object();
        std::string name, map_id;
        try {
            name = obj["userName"].as_string().c_str();
            map_id = obj["mapId"].as_string().c_str();
        } catch (...) {
            sendBadRequest(req, std::forward<Send>(send), "Join game request parse error");
            return;
        }

        if (name.empty()) {
            sendBadRequest(req, std::forward<Send>(send), "Invalid name");
            return;
        }

        const model::Map* map = game_.FindMap(model::Map::Id{map_id});
        if (!map) {
            sendNotFound(req, std::forward<Send>(send), "mapNotFound", "Map not found");
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
            sendMethodNotAllowed(req, std::forward<Send>(send), "GET, HEAD");
            return;
        }

        if (req.method() == http::verb::head) {
            http::response<http::string_body> res{http::status::ok, req.version()};
            send(std::move(res));
            return;
        }

        auto auth_header = req[http::field::authorization];
        if (auth_header.empty() || auth_header.substr(0, 7) != "Bearer ") {
            sendUnauthorized(req, std::forward<Send>(send), "invalidToken", "Authorization header is missing");
            return;
        }

        std::string token = auth_header.substr(7);
        auto* player = players_.FindByToken(token);
        if (!player) {
            sendUnauthorized(req, std::forward<Send>(send), "unknownToken", "Player token has not been found");
            return;
        }

        auto map_id = player->GetMapId();
        json::object resp;
        for (auto* p : players_.GetPlayersByMap(map_id)) {
            resp[std::to_string(p->GetId())] = json::object{{"name", p->GetName()}};
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.body() = json::serialize(resp);
        res.prepare_payload();
        send(std::move(res));
    }

private:
    model::Game& game_;
    model::PlayerManager& players_;

    template <typename Send>
    void sendBadRequest(const http::request<http::string_body>& req, Send&& send, const std::string& msg) {
        sendError(req, std::forward<Send>(send), http::status::bad_request, "invalidArgument", msg);
    }

    template <typename Send>
    void sendNotFound(const http::request<http::string_body>& req, Send&& send,
                      const std::string& code, const std::string& msg) {
        sendError(req, std::forward<Send>(send), http::status::not_found, code, msg);
    }

    template <typename Send>
    void sendUnauthorized(const http::request<http::string_body>& req, Send&& send,
                          const std::string& code, const std::string& msg) {
        sendError(req, std::forward<Send>(send), http::status::unauthorized, code, msg);
    }

    template <typename Send>
    void sendMethodNotAllowed(const http::request<http::string_body>& req, Send&& send,
                              const std::string& allow) {
        http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
        res.set(http::field::allow, allow);
        sendErrorBody(res, "invalidMethod", "Invalid method");
        send(std::move(res));
    }

    template <typename Send>
    void sendError(const http::request<http::string_body>& req, Send&& send,
                   http::status status, const std::string& code, const std::string& msg) {
        http::response<http::string_body> res{status, req.version()};
        sendErrorBody(res, code, msg);
        send(std::move(res));
    }

    void sendErrorBody(http::response<http::string_body>& res,
                       const std::string& code, const std::string& msg) {
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        json::object body{
            {"code", code},
            {"message", msg}
        };
        res.body() = json::serialize(body);
        res.prepare_payload();
    }
};

} // namespace http_handler