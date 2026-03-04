#include <fstream>
#include <sstream>
#include <boost/json.hpp>

#include "json_loader.h"

namespace json_loader {

namespace json = boost::json;

model::Game LoadGame(const std::filesystem::path& json_path) {
    model::Game game;

    std::ifstream file(json_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file");
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json_text = buffer.str();

    json::value root = json::parse(json_text);
    json::object root_obj = root.as_object();
    json::array maps = root_obj.at("maps").as_array();

    for (const auto& map_val : maps) {
        json::object map_obj = map_val.as_object();

        std::string id = map_obj.at("id").as_string().c_str();
        std::string name = map_obj.at("name").as_string().c_str();

        model::Map map(model::Map::Id{id}, name);

        json::array roads = map_obj.at("roads").as_array();
        for (const auto& road_val : roads) {
            json::object road_obj = road_val.as_object();

            int x0 = road_obj.at("x0").as_int64();
            int y0 = road_obj.at("y0").as_int64();

            if (road_obj.contains("x1")) {
                int x1 = road_obj.at("x1").as_int64();
                map.AddRoad(model::Road(
                    model::Road::HORIZONTAL,
                    {x0, y0},
                    x1
                ));
            } else {
                int y1 = road_obj.at("y1").as_int64();
                map.AddRoad(model::Road(
                    model::Road::VERTICAL,
                    {x0, y0},
                    y1
                ));
            }
        }

        json::array buildings = map_obj.at("buildings").as_array();
        for (const auto& building_val : buildings) {
            json::object building_obj = building_val.as_object();

            int x = building_obj.at("x").as_int64();
            int y = building_obj.at("y").as_int64();
            int w = building_obj.at("w").as_int64();
            int h = building_obj.at("h").as_int64();

            map.AddBuilding(
                model::Building(
                    model::Rectangle{
                        {x, y},
                        {w, h}
                    }
                )
            );
        }

        json::array offices = map_obj.at("offices").as_array();
        for (const auto& office_val : offices) {
            json::object office_obj = office_val.as_object();

            std::string office_id =
                office_obj.at("id").as_string().c_str();

            int x = office_obj.at("x").as_int64();
            int y = office_obj.at("y").as_int64();
            int offset_x = office_obj.at("offsetX").as_int64();
            int offset_y = office_obj.at("offsetY").as_int64();

            map.AddOffice(
                model::Office(
                    model::Office::Id{office_id},
                    {x, y},
                    {offset_x, offset_y}
                )
            );
        }

        game.AddMap(map);
    }

    return game;
}

}  // namespace json_loader
