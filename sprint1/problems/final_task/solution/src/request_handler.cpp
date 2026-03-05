#include "request_handler.h"

namespace http_handler {

json::array SerializeRoads(const model::Map& map) {
    json::array roads;

    for (const auto& road : map.GetRoads()) {
        json::object obj;

        obj["x0"] = road.GetStart().x;
        obj["y0"] = road.GetStart().y;

        if (road.IsHorizontal()) {
            obj["x1"] = road.GetEnd().x;
        } else {
            obj["y1"] = road.GetEnd().y;
        }

        roads.push_back(obj);
    }

    return roads;
}

json::array SerializeBuildings(const model::Map& map) {
    json::array buildings;

    for (const auto& b : map.GetBuildings()) {
        json::object obj;

        obj["x"] = b.GetBounds().position.x;
        obj["y"] = b.GetBounds().position.y;
        obj["w"] = b.GetBounds().size.width;
        obj["h"] = b.GetBounds().size.height;

        buildings.push_back(obj);
    }

    return buildings;
}

json::array SerializeOffices(const model::Map& map) {
    json::array offices;

    for (const auto& office : map.GetOffices()) {
        json::object obj;

        obj["id"] = *office.GetId();
        obj["x"] = office.GetPosition().x;
        obj["y"] = office.GetPosition().y;
        obj["offsetX"] = office.GetOffset().dx;
        obj["offsetY"] = office.GetOffset().dy;

        offices.push_back(obj);
    }

    return offices;
}

http::response<http::string_body> MakeJsonResponse(
    http::status status,
    const json::value& body,
    unsigned version) {

    http::response<http::string_body> res{status, version};
    res.set(http::field::content_type, "application/json");
    res.body() = json::serialize(body);
    res.prepare_payload();

    return res;
}

http::response<http::string_body> MakeErrorResponse(
    http::status status,
    std::string_view code,
    std::string_view message,
    unsigned version) {

    json::object err;

    err["code"] = std::string(code);
    err["message"] = std::string(message);

    return MakeJsonResponse(status, err, version);
}

http::response<http::string_body>
RequestHandler::GetMaps(unsigned version) {

    json::array maps;

    for (const auto& map : game_.GetMaps()) {
        json::object obj;

        obj["id"] = *map.GetId();
        obj["name"] = map.GetName();

        maps.push_back(obj);
    }

    return MakeJsonResponse(http::status::ok, maps, version);
}

http::response<http::string_body>
RequestHandler::GetMap(const std::string& target, unsigned version) {

    std::string map_id = target.substr(std::string(api::MAP).size());

    const model::Map* map = game_.FindMap(model::Map::Id{map_id});

    if (!map) {
        return MakeErrorResponse(
            http::status::not_found,
            "mapNotFound",
            "Map not found",
            version
        );
    }

    json::object result;

    result["id"] = *map->GetId();
    result["name"] = map->GetName();
    result["roads"] = SerializeRoads(*map);
    result["buildings"] = SerializeBuildings(*map);
    result["offices"] = SerializeOffices(*map);

    return MakeJsonResponse(http::status::ok, result, version);
}

http::response<http::string_body>
RequestHandler::MakeBadRequest(unsigned version) {

    return MakeErrorResponse(
        http::status::bad_request,
        "badRequest",
        "Bad request",
        version
    );
}

http::response<http::string_body>
RequestHandler::MakeMethodNotAllowed(unsigned version) {

    http::response<http::string_body> res{
        http::status::method_not_allowed,
        version
    };

    res.prepare_payload();
    return res;
}

}  // namespace http_handler
