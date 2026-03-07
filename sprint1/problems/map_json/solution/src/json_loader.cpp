#include <fstream>
#include <sstream>
#include <boost/json.hpp>

#include "json_loader.h"

namespace json_loader {

namespace json = boost::json;

json::object ParseJson(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    try {
        return json::parse(buffer.str()).as_object();
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    }
}

void LoadRoads(model::Map& map, const json::object& map_obj) {
    if (!map_obj.contains("roads")) {
        return;
    }

    for (const auto& road_val : map_obj.at("roads").as_array()) {
        const auto& road_obj = road_val.as_object();

        int x0 = road_obj.at("x0").as_int64();
        int y0 = road_obj.at("y0").as_int64();

        if (road_obj.contains("x1")) {
            int x1 = road_obj.at("x1").as_int64();

            map.AddRoad(model::Road(
                model::Road::HORIZONTAL,
                {x0, y0},
                x1
            ));
        } else if (road_obj.contains("y1")) {
            int y1 = road_obj.at("y1").as_int64();

            map.AddRoad(model::Road(
                model::Road::VERTICAL,
                {x0, y0},
                y1
            ));
        }
    }
}

void LoadBuildings(model::Map& map, const json::object& map_obj) {
    if (!map_obj.contains("buildings")) {
        return;
    }

    for (const auto& building_val : map_obj.at("buildings").as_array()) {
        const auto& obj = building_val.as_object();

        map.AddBuilding(
            model::Building(
                model::Rectangle{
                    {obj.at("x").as_int64(), obj.at("y").as_int64()},
                    {obj.at("w").as_int64(), obj.at("h").as_int64()}
                }
            )
        );
    }
}

void LoadOffices(model::Map& map, const json::object& map_obj) {
    if (!map_obj.contains("offices")) {
        return;
    }

    for (const auto& office_val : map_obj.at("offices").as_array()) {
        const auto& obj = office_val.as_object();

        std::string id = obj.at("id").as_string().c_str();

        map.AddOffice(
            model::Office(
                model::Office::Id{id},
                {obj.at("x").as_int64(), obj.at("y").as_int64()},
                {obj.at("offsetX").as_int64(), obj.at("offsetY").as_int64()}
            )
        );
    }
}

model::Map LoadMap(const json::object& map_obj) {
    std::string id = map_obj.at("id").as_string().c_str();
    std::string name = map_obj.at("name").as_string().c_str();

    model::Map map(model::Map::Id{id}, name);

    LoadRoads(map, map_obj);
    LoadBuildings(map, map_obj);
    LoadOffices(map, map_obj);

    return map;
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    model::Game game;

    json::object root = ParseJson(json_path);

    if (!root.contains("maps")) {
        throw std::runtime_error("Config must contain maps");
    }

    for (const auto& map_val : root.at("maps").as_array()) {
        game.AddMap(LoadMap(map_val.as_object()));
    }

    return game;
}

}  // namespace json_loader
