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
            http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
            res.set(http::field::allow, "POST");
            send(std::move(res));
            return;
        }

        json::value body;
        try {
            body = json::parse(req.body());
        } catch (...) {
            http::response<http::string_body> res{http::status::bad_request, req.version()};
            send(std::move(res));
            return;
        }

        auto obj = body.as_object();
        std::string name = obj["userName"].as_string().c_str();
        std::string map_id = obj["mapId"].as_string().c_str();

        const model::Map* map = game_.FindMap(model::Map::Id{map_id});
        if (!map) {
            http::response<http::string_body> res{http::status::not_found, req.version()};
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
        res.body() = json::serialize(resp);
        res.prepare_payload();

        send(std::move(res));
    }

    template <typename Send>
    void HandlePlayers(const http::request<http::string_body>& req, Send&& send) {
        if (req.method() != http::verb::get) {
            http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
            res.set(http::field::allow, "GET");
            send(std::move(res));
            return;
        }

        json::array players_array;
        for (auto* player : players_.GetPlayersByMap(model::Map::Id{"some_map_id"})) {
            json::object obj{
                {"playerId", player->GetId()},
                {"userName", player->GetName()}
            };
            players_array.push_back(obj);
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.body() = json::serialize(players_array);
        res.prepare_payload();

        send(std::move(res));
    }

private:
    model::Game& game_;
    model::PlayerManager& players_;
};

} // namespace http_handler