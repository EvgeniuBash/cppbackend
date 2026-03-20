#pragma once

#include "model.h"
#include <boost/json.hpp>
#include "http_server.h"
#include "player.h"
#include <string>
#include <string_view>

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
            json::object res_obj{
                {"code", "invalidMethod"},
                {"message", "Only POST method is expected"}
            };
            http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
            res.set(http::field::allow, "POST");
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(res_obj);
            res.prepare_payload();
            send(std::move(res));
            return;
        }

        json::value body;
        try {
            body = json::parse(req.body());
        } catch (...) {
            json::object res_obj{
                {"code", "invalidArgument"},
                {"message", "Join game request parse error"}
            };
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(res_obj);
            res.prepare_payload();
            send(std::move(res));
            return;
        }

        auto obj = body.as_object();
        std::string name;
        std::string map_id;

        try {
            name = std::string(obj["userName"].as_string());
            map_id = std::string(obj["mapId"].as_string());
        } catch (...) {
            json::object res_obj{
                {"code", "invalidArgument"},
                {"message", "Join game request parse error"}
            };
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(res_obj);
            res.prepare_payload();
            send(std::move(res));
            return;
        }

        if (name.empty()) {
            json::object res_obj{
                {"code", "invalidArgument"},
                {"message", "Invalid name"}
            };
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(res_obj);
            res.prepare_payload();
            send(std::move(res));
            return;
        }

        const model::Map* map = game_.FindMap(model::Map::Id{map_id});
        if (!map) {
            json::object res_obj{
                {"code", "mapNotFound"},
                {"message", "Map not found"}
            };
            http::response<http::string_body> res{http::status::not_found, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(res_obj);
            res.prepare_payload();
            send(std::move(res));
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
            json::object res_obj{
                {"code", "invalidMethod"},
                {"message", "Invalid method"}
            };
            http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
            res.set(http::field::allow, "GET, HEAD");
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(res_obj);
            res.prepare_payload();
            send(std::move(res));
            return;
        }

        auto auth_it = req.find(http::field::authorization);
        if (auth_it == req.end() || !auth_it->value().starts_with("Bearer ")) {
            json::object res_obj{
                {"code", "invalidToken"},
                {"message", "Authorization header is missing or invalid"}
            };
            http::response<http::string_body> res{http::status::unauthorized, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(res_obj);
            res.prepare_payload();
            send(std::move(res));
            return;
        }

        std::string auth_header = std::string(auth_it->value());
        std::string token = std::string(auth_header.substr(7));

        model::Player* player = players_.FindByToken(token);
        if (!player) {
            json::object res_obj{
                {"code", "unknownToken"},
                {"message", "Player token has not been found"}
            };
            http::response<http::string_body> res{http::status::unauthorized, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = json::serialize(res_obj);
            res.prepare_payload();
            send(std::move(res));
            return;
        }

        json::object resp;
        for (auto* p : players_.GetPlayersByMap(player->GetMapId())) {
            json::object obj{{"name", p->GetName()}};
            resp[std::to_string(p->GetId())] = obj;
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
};

} // namespace http_handler