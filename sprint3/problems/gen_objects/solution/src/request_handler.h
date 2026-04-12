#pragma once

#include <boost/json.hpp>
#include "http_server.h"
#include "model.h"
#include "api_handler.h"
#include <filesystem>
#include <string>
#include <string_view>
#include <algorithm>
#include <optional>

namespace fs = std::filesystem;

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

namespace {

const std::string MAPS_ENDPOINT = "/api/v1/maps";
const std::string MAP_ENDPOINT_PREFIX = "/api/v1/maps/";
const std::string GAME_STATE = "/api/v1/game/state";
const std::string GAME_ACTION = "/api/v1/game/player/action";
const std::string GAME_TICK = "/api/v1/game/tick";
const std::string GAME_JOIN = "/api/v1/game/join";
const std::string GAME_PLAYERS = "/api/v1/game/players";
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
} // namespace json_keys

std::string UrlDecode(std::string_view str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];

        if (c == '%' && i + 2 < str.size()) {
            std::string hex{str.substr(i + 1, 2)};
            result += static_cast<char>(std::stoi(hex, nullptr, 16));
            i += 2;
        } else if (c == '+') {
            result += ' ';
        } else {
            result += c;
        }
    }
    return result;
}

std::string GetMimeType(const fs::path& path) {
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if (ext == ".htm" || ext == ".html") return "text/html";
    if (ext == ".css") return "text/css";
    if (ext == ".txt") return "text/plain";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";

    return "application/octet-stream";
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

} // namespace

class RequestHandler {
public:
    explicit RequestHandler(model::Game& game,
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
        , randomize_spawn_(randomize_spawn)
        , tick_period_(tick_period)
    {}

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req,
                    Send&& send) {

        const std::string target = std::string(req.target());

        // ===== GAME API =====

        if (target == GAME_JOIN) {
            api_handler_.HandleJoin(req, std::forward<Send>(send));
            return;
        }

        if (target == GAME_PLAYERS) {
            api_handler_.HandlePlayers(req, std::forward<Send>(send));
            return;
        }

        if (target == GAME_STATE) {
            api_handler_.HandleState(req, std::forward<Send>(send));
            return;
        }

        if (target == GAME_ACTION) {
            api_handler_.HandleAction(req, std::forward<Send>(send));
            return;
        }

        if (target == GAME_TICK) {
            api_handler_.HandleTick(req, std::forward<Send>(send));
            return;
        }

        // запрещаем tick при fixed tick
        if (tick_period_.has_value() && target == GAME_TICK) {
            json::object body{
                {"code", "badRequest"},
                {"message", "Invalid endpoint"}
            };

            http::response<http::string_body> response{
                http::status::bad_request, req.version()
            };

            response.set(http::field::content_type, "application/json");
            response.body() = json::serialize(body);
            response.prepare_payload();

            send(std::move(response));
            return;
        }

        // ===== MAPS =====

        if (req.method() == http::verb::get && target == MAPS_ENDPOINT) {
            json::array arr;

            for (const auto& map : game_.GetMaps()) {
                json::object obj;
                obj["id"] = *map.GetId();
                obj["name"] = map.GetName();
                arr.push_back(obj);
            }

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.body() = json::serialize(arr);
            res.prepare_payload();
            send(std::move(res));
            return;
        }

        if (req.method() == http::verb::get && target.starts_with(MAP_ENDPOINT_PREFIX)) {
            std::string map_id = target.substr(MAP_ENDPOINT_PREFIX.size());
            const model::Map* map = game_.FindMap(model::Map::Id{map_id});

            if (!map) {
                json::object err{
                    {"code", "mapNotFound"},
                    {"message", "Map not found"}
                };

                http::response<http::string_body> res{http::status::not_found, req.version()};
                res.set(http::field::content_type, "application/json");
                res.body() = json::serialize(err);
                res.prepare_payload();
                send(std::move(res));
                return;
            }

            json::array roads, buildings, offices;

            for (const auto& r : map->GetRoads())
                roads.push_back(SerializeRoad(r));

            for (const auto& b : map->GetBuildings())
                buildings.push_back(SerializeBuilding(b));

            for (const auto& o : map->GetOffices())
                offices.push_back(SerializeOffice(o));

            json::object result;
            result["id"] = *map->GetId();
            result["name"] = map->GetName();
            result["roads"] = std::move(roads);
            result["buildings"] = std::move(buildings);
            result["offices"] = std::move(offices);

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.body() = json::serialize(result);
            res.prepare_payload();
            send(std::move(res));
            return;
        }

        // ===== STATIC =====

        if (req.method() != http::verb::get) {
            json::object err{
                {"code", "invalidMethod"},
                {"message", "Invalid method"}
            };

            http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
            res.set(http::field::content_type, "application/json");
            res.body() = json::serialize(err);
            res.prepare_payload();
            send(std::move(res));
            return;
        }

        auto decoded = UrlDecode(target);
        fs::path path = static_root_ / (decoded == "/" ? "index.html" : decoded.substr(1));

        if (!fs::exists(path)) {
            http::response<http::string_body> res{http::status::not_found, req.version()};
            res.body() = "404 Not Found";
            res.prepare_payload();
            send(std::move(res));
            return;
        }

        http::file_body::value_type file;
        beast::error_code ec;
        file.open(path.c_str(), beast::file_mode::read, ec);

        if (ec) {
            http::response<http::string_body> res{http::status::internal_server_error, req.version()};
            res.body() = "500 Error";
            res.prepare_payload();
            send(std::move(res));
            return;
        }

        auto size = file.size();

        http::response<http::file_body> res{
            std::piecewise_construct,
            std::make_tuple(std::move(file)),
            std::make_tuple(http::status::ok, req.version())
        };

        res.set(http::field::content_type, GetMimeType(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());

        send(std::move(res));
    }

private:
    model::Game& game_;
    model::PlayerManager& players_;
    std::filesystem::path static_root_;
    extra_data::Storage& extra_data_;
    ApiHandler api_handler_;
    bool randomize_spawn_;
    std::optional<int> tick_period_;
};

} // namespace http_handler
