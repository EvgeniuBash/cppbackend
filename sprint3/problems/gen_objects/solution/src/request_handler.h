#pragma once

#include <boost/json.hpp>
#include "http_server.h"
#include "model.h"
#include "api_handler.h"
#include "map_extra_data.h"

#include <filesystem>
#include <string>
#include <optional>

namespace http_handler {

namespace http = boost::beast::http;
namespace json = boost::json;

class RequestHandler {
public:
    RequestHandler(model::Game& game,
                   model::PlayerManager& players,
                   std::filesystem::path static_root,
                   extra_data::Storage& extra_data,
                   bool randomize_spawn,
                   std::optional<int> tick_period)
        : game_(game)
        , players_(players)
        , static_root_(std::move(static_root))
        , extra_data_(extra_data)
        , api_handler_(game_, players_, extra_data_, randomize_spawn)
        , tick_period_(tick_period) {}

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {

        std::string target = std::string(req.target());

        // ================= GAME =================

        if (target == "/api/v1/game/join") {
            api_handler_.HandleJoin(req, send);
            return;
        }

        if (target == "/api/v1/game/players") {
            api_handler_.HandlePlayers(req, send);
            return;
        }

        if (target == "/api/v1/game/state") {
            if (req.method() != http::verb::get && req.method() != http::verb::head) {
                SendError(send, req, http::status::method_not_allowed);
                return;
            }
            api_handler_.HandleState(req, send);
            return;
        }

        if (target == "/api/v1/game/player/action") {
            api_handler_.HandleAction(req, send);
            return;
        }

        if (target == "/api/v1/game/tick") {
            if (tick_period_) {
                SendError(send, req, http::status::bad_request);
                return;
            }
            api_handler_.HandleTick(req, send);
            return;
        }

        // ================= MAPS =================

        if (target == "/api/v1/maps") {
            if (req.method() != http::verb::get && req.method() != http::verb::head) {
                SendError(send, req, http::status::method_not_allowed);
                return;
            }

            json::array maps;

            for (const auto& map : game_.GetMaps()) {
                maps.push_back({
                    {"id", *map.GetId()},
                    {"name", map.GetName()}
                });
            }

            SendJson(send, req, maps);
            return;
        }

        // ===== /api/v1/maps/{id} =====
        if (target.starts_with("/api/v1/maps/")) {
            std::string id = target.substr(13);

            const model::Map* map = game_.FindMap(model::Map::Id{id});

            // ❗ сначала проверяем существование карты
            if (!map) {
                json::object err{
                    {"code", "mapNotFound"},
                    {"message", "Map not found"}
                };
                SendJson(send, req, err, http::status::not_found);
                return;
            }

            // ❗ потом метод
            if (req.method() != http::verb::get && req.method() != http::verb::head) {
                SendError(send, req, http::status::method_not_allowed);
                return;
            }

            json::array roads, buildings, offices;

            for (const auto& r : map->GetRoads()) {
                json::object o;
                o["x0"] = r.GetStart().x;
                o["y0"] = r.GetStart().y;

                if (r.IsHorizontal())
                    o["x1"] = r.GetEnd().x;
                else
                    o["y1"] = r.GetEnd().y;

                roads.push_back(o);
            }

            for (const auto& b : map->GetBuildings()) {
                json::object o;
                o["x"] = b.GetBounds().position.x;
                o["y"] = b.GetBounds().position.y;
                o["w"] = b.GetBounds().size.width;
                o["h"] = b.GetBounds().size.height;
                buildings.push_back(o);
            }

            for (const auto& o : map->GetOffices()) {
                json::object obj;
                obj["id"] = *o.GetId();
                obj["x"] = o.GetPosition().x;
                obj["y"] = o.GetPosition().y;
                obj["offsetX"] = o.GetOffset().dx;
                obj["offsetY"] = o.GetOffset().dy;
                offices.push_back(obj);
            }

            json::object result{
                {"id", *map->GetId()},
                {"name", map->GetName()},
                {"roads", roads},
                {"buildings", buildings},
                {"offices", offices}
            };

            // ✅ lootTypes
            auto extra = extra_data_.Get(map->GetId());
            if (!extra.empty()) {
                result["lootTypes"] = extra;
            }

            SendJson(send, req, result);
            return;
        }

        // ================= DEFAULT =================

        SendError(send, req, http::status::bad_request);
    }

private:
    // ================= HELPERS =================

    template <typename Send>
    void SendJson(Send&& send, const auto& req, const json::value& body,
                  http::status status = http::status::ok) {

        http::response<http::string_body> res{status, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");

        if (req.method() != http::verb::head) {
            res.body() = json::serialize(body);
        }

        res.prepare_payload();
        send(std::move(res));
    }

    template <typename Send>
    void SendError(Send&& send, const auto& req, http::status status) {

        std::string code;
        std::string message;

        if (status == http::status::method_not_allowed) {
            code = "invalidMethod";
            message = "Invalid method";
        } else if (status == http::status::bad_request) {
            code = "invalidRequest";
            message = "Bad request";
        } else {
            code = "invalidRequest";
            message = "Invalid request";
        }

        json::object body{
            {"code", code},
            {"message", message}
        };

        http::response<http::string_body> res{status, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");

        if (status == http::status::method_not_allowed) {
            res.set(http::field::allow, "GET, HEAD");
        }

        if (req.method() != http::verb::head) {
            res.body() = json::serialize(body);
        }

        res.prepare_payload();
        send(std::move(res));
    }

private:
    model::Game& game_;
    model::PlayerManager& players_;
    std::filesystem::path static_root_;
    extra_data::Storage& extra_data_;
    ApiHandler api_handler_;
    std::optional<int> tick_period_;
};

} // namespace http_handler
