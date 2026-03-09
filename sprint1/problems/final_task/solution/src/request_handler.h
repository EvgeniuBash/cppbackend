#pragma once

#include <boost/json.hpp>
#include "http_server.h"
#include "model.h"

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

namespace {

const std::string MAPS_ENDPOINT = "/api/v1/maps";
const std::string MAP_ENDPOINT_PREFIX = "/api/v1/maps/";
const std::string API_PREFIX = "/api/";

namespace json_keys {

const char* ID = "id";
const char* NAME = "name";

const char* X0 = "x0";
const char* Y0 = "y0";
const char* X1 = "x1";
const char* Y1 = "y1";

const char* X = "x";
const char* Y = "y";
const char* W = "w";
const char* H = "h";

const char* OFFSET_X = "offsetX";
const char* OFFSET_Y = "offsetY";

}

json::object SerializeRoad(const model::Road& road) {
    json::object obj;

    obj[json_keys::X0] = road.GetStart().x;
    obj[json_keys::Y0] = road.GetStart().y;

    if (road.IsHorizontal())
        obj[json_keys::X1] = road.GetEnd().x;
    else
        obj[json_keys::Y1] = road.GetEnd().y;

    return obj;
}

json::object SerializeBuilding(const model::Building& building) {
    json::object obj;

    obj[json_keys::X] = building.GetBounds().position.x;
    obj[json_keys::Y] = building.GetBounds().position.y;
    obj[json_keys::W] = building.GetBounds().size.width;
    obj[json_keys::H] = building.GetBounds().size.height;

    return obj;
}

json::object SerializeOffice(const model::Office& office) {
    json::object obj;

    obj[json_keys::ID] = *office.GetId();
    obj[json_keys::X] = office.GetPosition().x;
    obj[json_keys::Y] = office.GetPosition().y;
    obj[json_keys::OFFSET_X] = office.GetOffset().dx;
    obj[json_keys::OFFSET_Y] = office.GetOffset().dy;

    return obj;
}

}

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game)
        : game_{game} {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
                    Send&& send) {

        std::string target = std::string(req.target());

        if (req.method() != http::verb::get) {
            http::response<http::string_body> response{
                http::status::method_not_allowed, req.version()};
            response.prepare_payload();
            send(std::move(response));
            return;
        }

        if (target == MAPS_ENDPOINT) {
            json::array maps_array;

            for (const auto& map : game_.GetMaps()) {
                json::object map_obj;

                map_obj[json_keys::ID] = *map.GetId();
                map_obj[json_keys::NAME] = map.GetName();

                maps_array.push_back(map_obj);
            }

            http::response<http::string_body> response{
                http::status::ok, req.version()};

            response.set(http::field::content_type, "application/json");
            response.body() = json::serialize(maps_array);
            response.prepare_payload();

            send(std::move(response));
            return;
        }

        if (target.starts_with(MAP_ENDPOINT_PREFIX)) {

            std::string map_id = target.substr(MAP_ENDPOINT_PREFIX.size());

            const model::Map* map =
                game_.FindMap(model::Map::Id{map_id});

            if (!map) {
                json::object error;

                error["code"] = "mapNotFound";
                error["message"] = "Map not found";

                http::response<http::string_body> response{
                    http::status::not_found, req.version()};

                response.set(http::field::content_type, "application/json");
                response.body() = json::serialize(error);
                response.prepare_payload();

                send(std::move(response));
                return;
            }

            json::array roads_array;
            for (const auto& road : map->GetRoads())
                roads_array.push_back(SerializeRoad(road));

            json::array buildings_array;
            for (const auto& building : map->GetBuildings())
                buildings_array.push_back(SerializeBuilding(building));

            json::array offices_array;
            for (const auto& office : map->GetOffices())
                offices_array.push_back(SerializeOffice(office));

            json::object result;

            result[json_keys::ID] = *map->GetId();
            result[json_keys::NAME] = map->GetName();
            result["roads"] = roads_array;
            result["buildings"] = buildings_array;
            result["offices"] = offices_array;

            http::response<http::string_body> response{
                http::status::ok, req.version()};

            response.set(http::field::content_type, "application/json");
            response.body() = json::serialize(result);
            response.prepare_payload();

            send(std::move(response));
            return;
        }

        if (target.starts_with(API_PREFIX)) {

            json::object error;

            error["code"] = "badRequest";
            error["message"] = "Bad request";

            http::response<http::string_body> response{
                http::status::bad_request, req.version()};

            response.set(http::field::content_type, "application/json");
            response.body() = json::serialize(error);
            response.prepare_payload();

            send(std::move(response));
        }
    }

private:
    model::Game& game_;
};

}
