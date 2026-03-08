#pragma once
#include <boost/json.hpp>
#include <string_view>
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

    constexpr char ID[] = "id";
    constexpr char NAME[] = "name";
    constexpr char X[] = "x";
    constexpr char Y[] = "y";
    constexpr char X0[] = "x0";
    constexpr char Y0[] = "y0";
    constexpr char X1[] = "x1";
    constexpr char Y1[] = "y1";
    constexpr char W[] = "w";
    constexpr char H[] = "h";
    constexpr char OFFSET_X[] = "offsetX";
    constexpr char OFFSET_Y[] = "offsetY";
    constexpr char CODE[] = "code";
    constexpr char MESSAGE[] = "message";
    constexpr char ROADS[] = "roads";
    constexpr char BUILDINGS[] = "buildings";
    constexpr char OFFICES[] = "offices";

    json::object SerializeRoad(const model::Road& road) {
        json::object obj;
        obj[X0] = road.GetStart().x;
        obj[Y0] = road.GetStart().y;

        if (road.IsHorizontal()) {
            obj[X1] = road.GetEnd().x;
        } else {
            obj[Y1] = road.GetEnd().y;
        }

        return obj;
    }

    json::object SerializeBuilding(const model::Building& building) {
        json::object obj;
        obj[X] = building.GetBounds().position.x;
        obj[Y] = building.GetBounds().position.y;
        obj[W] = building.GetBounds().size.width;
        obj[H] = building.GetBounds().size.height;
        return obj;
    }

    json::object SerializeOffice(const model::Office& office) {
        json::object obj;
        obj[ID] = *office.GetId();
        obj[X] = office.GetPosition().x;
        obj[Y] = office.GetPosition().y;
        obj[OFFSET_X] = office.GetOffset().dx;
        obj[OFFSET_Y] = office.GetOffset().dy;
        return obj;
    }

    json::object SerializeMapPreview(const model::Map& map) {
        json::object obj;
        obj[ID] = *map.GetId();
        obj[NAME] = map.GetName();
        return obj;
    }

    json::object SerializeMapFull(const model::Map& map) {
        json::object result = SerializeMapPreview(map);

        json::array roads_array;
        for (const auto& road : map.GetRoads()) {
            roads_array.push_back(SerializeRoad(road));
        }
        result[ROADS] = roads_array;

        json::array buildings_array;
        for (const auto& building : map.GetBuildings()) {
            buildings_array.push_back(SerializeBuilding(building));
        }
        result[BUILDINGS] = buildings_array;

        json::array offices_array;
        for (const auto& office : map.GetOffices()) {
            offices_array.push_back(SerializeOffice(office));
        }
        result[OFFICES] = offices_array;

        return result;
    }

    json::object CreateErrorResponse(const std::string& code, const std::string& message) {
        json::object error;
        error[CODE] = code;
        error[MESSAGE] = message;
        return error;
    }

    template<typename Send>
    void SendJsonResponse(Send&& send, http::status status, 
                          const json::serialize& body, unsigned version) {
        http::response<http::string_body> response{status, version};
        response.set(http::field::content_type, "application/json");
        response.body() = body;
        response.prepare_payload();
        send(std::move(response));
    }
}

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game) : game_{game} {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        std::string target = std::string(req.target());

        if (req.method() != http::verb::get) {
            return SendJsonResponse(std::forward<Send>(send), 
                http::status::method_not_allowed, 
                json::serialize(CreateErrorResponse("methodNotAllowed", "Method not allowed")),
                req.version());
        }

        if (target == MAPS_ENDPOINT) {
            HandleGetMaps(std::forward<Send>(send), req.version());
            return;
        }

        if (target.starts_with(MAP_ENDPOINT_PREFIX)) {
            std::string map_id = target.substr(MAP_ENDPOINT_PREFIX.size());
            HandleGetMap(map_id, std::forward<Send>(send), req.version());
            return;
        }

        if (target.starts_with(API_PREFIX)) {
            SendJsonResponse(std::forward<Send>(send), 
                http::status::bad_request,
                json::serialize(CreateErrorResponse("badRequest", "Bad request")),
                req.version());
            return;
        }
    }

private:
    template<typename Send>
    void HandleGetMaps(Send&& send, unsigned version) {
        json::array maps_array;
        for (const auto& map : game_.GetMaps()) {
            maps_array.push_back(SerializeMapPreview(map));
        }
        SendJsonResponse(std::forward<Send>(send), 
            http::status::ok, 
            json::serialize(maps_array),
            version);
    }

    template<typename Send>
    void HandleGetMap(const std::string& map_id, Send&& send, unsigned version) {
        const model::Map* map = game_.FindMap(model::Map::Id{map_id});
        
        if (!map) {
            SendJsonResponse(std::forward<Send>(send), 
                http::status::not_found,
                json::serialize(CreateErrorResponse("mapNotFound", "Map not found")),
                version);
            return;
        }

        SendJsonResponse(std::forward<Send>(send), 
            http::status::ok,
            json::serialize(SerializeMapFull(*map)),
            version);
    }

    model::Game& game_;
};

} // namespace http_handler
