#pragma once

#include <boost/json.hpp>
#include "http_server.h"
#include "model.h"

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
                    Send&& send) {

        std::string target = std::string(req.target());

        if (req.method() != http::verb::get) {
            http::response<http::string_body> response{
                http::status::method_not_allowed,
                req.version()
            };
            response.prepare_payload();
            send(std::move(response));
            return;
        }

        if (target == "/api/v1/maps") {

            json::array maps_array;

            for (const auto& map : game_.GetMaps()) {
                json::object map_obj;
                map_obj["id"] = *map.GetId();
                map_obj["name"] = map.GetName();
                maps_array.push_back(map_obj);
            }

            http::response<http::string_body> response{
                http::status::ok,
                req.version()
            };

            response.set(http::field::content_type, "application/json");
            response.body() = json::serialize(maps_array);
            response.prepare_payload();

            send(std::move(response));
            return;
        }

        if (target.starts_with("/api/v1/maps/")) {

            std::string map_id =
                target.substr(std::string("/api/v1/maps/").size());

            const model::Map* map =
                game_.FindMap(model::Map::Id{map_id});

            if (!map) {
                json::object error;
                error["code"] = "mapNotFound";
                error["message"] = "Map not found";

                http::response<http::string_body> response{
                    http::status::not_found,
                    req.version()
                };

                response.set(http::field::content_type, "application/json");
                response.body() = json::serialize(error);
                response.prepare_payload();

                send(std::move(response));
                return;
            }

            json::array roads_array;
            for (const auto& road : map->GetRoads()) {
                json::object obj;

                obj["x0"] = road.GetStart().x;
                obj["y0"] = road.GetStart().y;

                if (road.IsHorizontal())
                    obj["x1"] = road.GetEnd().x;
                else
                    obj["y1"] = road.GetEnd().y;

                roads_array.push_back(obj);
            }

            json::array buildings_array;
            for (const auto& building : map->GetBuildings()) {
                json::object obj;

                obj["x"] = building.GetBounds().position.x;
                obj["y"] = building.GetBounds().position.y;
                obj["w"] = building.GetBounds().size.width;
                obj["h"] = building.GetBounds().size.height;

                buildings_array.push_back(obj);
            }

            json::array offices_array;
            for (const auto& office : map->GetOffices()) {
                json::object obj;

                obj["id"] = *office.GetId();
                obj["x"] = office.GetPosition().x;
                obj["y"] = office.GetPosition().y;
                obj["offsetX"] = office.GetOffset().dx;
                obj["offsetY"] = office.GetOffset().dy;

                offices_array.push_back(obj);
            }

            json::object result;
            result["id"] = *map->GetId();
            result["name"] = map->GetName();
            result["roads"] = roads_array;
            result["buildings"] = buildings_array;
            result["offices"] = offices_array;

            http::response<http::string_body> response{
                http::status::ok,
                req.version()
            };

            response.set(http::field::content_type, "application/json");
            response.body() = json::serialize(result);
            response.prepare_payload();

            send(std::move(response));
            return;
        }

        if (target.starts_with("/api/")) {
            json::object error;
            error["code"] = "badRequest";
            error["message"] = "Bad request";

            http::response<http::string_body> response{
                http::status::bad_request,
                req.version()
            };

            response.set(http::field::content_type, "application/json");
            response.body() = json::serialize(error);
            response.prepare_payload();

            send(std::move(response));
            return;
        }
    }

private:
    model::Game& game_;
};

}  // namespace http_handler
