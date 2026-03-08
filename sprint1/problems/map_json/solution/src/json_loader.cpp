#include <fstream>
#include <sstream>
#include <boost/json.hpp>
#include "json_loader.h"

namespace json_loader {
namespace json = boost::json;

namespace {
    constexpr char ID[] = "id";
    constexpr char NAME[] = "name";
    constexpr char X0[] = "x0";
    constexpr char Y0[] = "y0";
    constexpr char X1[] = "x1";
    constexpr char Y1[] = "y1";
    constexpr char X[] = "x";
    constexpr char Y[] = "y";
    constexpr char W[] = "w";
    constexpr char H[] = "h";
    constexpr char OFFSET_X[] = "offsetX";
    constexpr char OFFSET_Y[] = "offsetY";
    constexpr char MAPS[] = "maps";
    constexpr char ROADS[] = "roads";
    constexpr char BUILDINGS[] = "buildings";
    constexpr char OFFICES[] = "offices";

    model::Road ParseRoad(const json::object& road_obj) {
        int x0 = road_obj.at(X0).as_int64();
        int y0 = road_obj.at(Y0).as_int64();

        if (road_obj.contains(X1)) {
            int x1 = road_obj.at(X1).as_int64();
            return model::Road(model::Road::HORIZONTAL, {x0, y0}, x1);
        } else {
            int y1 = road_obj.at(Y1).as_int64();
            return model::Road(model::Road::VERTICAL, {x0, y0}, y1);
        }
    }

    model::Building ParseBuilding(const json::object& building_obj) {
        int x = building_obj.at(X).as_int64();
        int y = building_obj.at(Y).as_int64();
        int w = building_obj.at(W).as_int64();
        int h = building_obj.at(H).as_int64();

        return model::Building(model::Rectangle{{x, y}, {w, h}});
    }

    model::Office ParseOffice(const json::object& office_obj) {
        std::string id = office_obj.at(ID).as_string().c_str();
        int x = office_obj.at(X).as_int64();
        int y = office_obj.at(Y).as_int64();
        int offset_x = office_obj.at(OFFSET_X).as_int64();
        int offset_y = office_obj.at(OFFSET_Y).as_int64();

        return model::Office(model::Office::Id{id}, {x, y}, {offset_x, offset_y});
    }

    model::Map ParseMap(const json::object& map_obj) {
        std::string id = map_obj.at(ID).as_string().c_str();
        std::string name = map_obj.at(NAME).as_string().c_str();
        
        model::Map map(model::Map::Id{id}, name);

        for (const auto& road_val : map_obj.at(ROADS).as_array()) {
            map.AddRoad(ParseRoad(road_val.as_object()));
        }

        for (const auto& building_val : map_obj.at(BUILDINGS).as_array()) {
            map.AddBuilding(ParseBuilding(building_val.as_object()));
        }

        for (const auto& office_val : map_obj.at(OFFICES).as_array()) {
            map.AddOffice(ParseOffice(office_val.as_object()));
        }

        return map;
    }
} 

model::Game LoadGame(const std::filesystem::path& json_path) {
    model::Game game;

    std::ifstream file(json_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json_text = buffer.str();

    json::value root;
    try {
        root = json::parse(json_text);
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    }

    json::object root_obj = root.as_object();
    
    for (const auto& map_val : root_obj.at(MAPS).as_array()) {
        game.AddMap(ParseMap(map_val.as_object()));
    }

    return game;
}

} // namespace json_loader
