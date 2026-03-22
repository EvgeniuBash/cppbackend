#include <fstream>
#include <sstream>
#include <boost/json.hpp>

#include "json_loader.h"

namespace json_loader {

namespace json = boost::json;

namespace {

model::Road ParseRoad(const json::object& road_obj) {

    int x0 = road_obj.at("x0").as_int64();
    int y0 = road_obj.at("y0").as_int64();

    if (road_obj.contains("x1")) {

        int x1 = road_obj.at("x1").as_int64();

        return model::Road(model::Road::HORIZONTAL, {x0, y0}, x1);
    }
    else {

        int y1 = road_obj.at("y1").as_int64();

        return model::Road(model::Road::VERTICAL, {x0, y0}, y1);
    }
}

model::Building ParseBuilding(const json::object& obj) {

    int x = obj.at("x").as_int64();
    int y = obj.at("y").as_int64();
    int w = obj.at("w").as_int64();
    int h = obj.at("h").as_int64();

    return model::Building(model::Rectangle{{x, y}, {w, h}});
}

model::Office ParseOffice(const json::object& obj) {

    std::string id = obj.at("id").as_string().c_str();

    int x = obj.at("x").as_int64();
    int y = obj.at("y").as_int64();

    int offset_x = obj.at("offsetX").as_int64();
    int offset_y = obj.at("offsetY").as_int64();

    return model::Office(model::Office::Id{id}, {x, y}, {offset_x, offset_y});
}

}

model::Game LoadGame(const std::filesystem::path& json_path) {

    model::Game game;

    std::ifstream file(json_path);

    if (!file.is_open())
        throw std::runtime_error("Failed to open config file");

    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string json_text = buffer.str();

    json::value root;

    try {
        root = json::parse(json_text);
    }
    catch (const std::exception& e) {
        throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    }

    json::object root_obj = root.as_object();

    double default_speed = 1.0;

    if (root_obj.contains("defaultDogSpeed")) {
        default_speed = root_obj.at("defaultDogSpeed").as_double();
    }

    json::array maps = root_obj.at("maps").as_array();

    for (const auto& map_val : maps) {

        json::object map_obj = map_val.as_object();

        std::string id = map_obj.at("id").as_string().c_str();
        std::string name = map_obj.at("name").as_string().c_str();

        model::Map map(model::Map::Id{id}, name);

        double speed = default_speed;

        if (map_obj.contains("dogSpeed")) {
            speed = map_obj.at("dogSpeed").as_double();
        }

        map.SetDogSpeed(speed);

        json::array roads = map_obj.at("roads").as_array();
        for (const auto& road_val : roads)
            map.AddRoad(ParseRoad(road_val.as_object()));

        json::array buildings = map_obj.at("buildings").as_array();
        for (const auto& building_val : buildings)
            map.AddBuilding(ParseBuilding(building_val.as_object()));

        json::array offices = map_obj.at("offices").as_array();
        for (const auto& office_val : offices)
            map.AddOffice(ParseOffice(office_val.as_object()));

        game.AddMap(map);
    }

    return game;
}

}
