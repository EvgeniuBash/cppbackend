#pragma once

#include <boost/json.hpp>
#include "http_server.h"
#include "model.h"
#include "api_handler.h"
#include "player.h"
#include <string>
#include <string_view>
#include <algorithm>
#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

namespace {

const std::string MAPS_ENDPOINT = "/api/v1/maps";
const std::string MAP_ENDPOINT_PREFIX = "/api/v1/maps/";
const std::string API_PREFIX = "/api/";
const std::string GAME_STATE = "/api/v1/game/state";
const std::string GAME_ACTION = "/api/v1/game/player/action";
const std::string GAME_TICK = "/api/v1/game/tick";
const std::string GAME_JOiN = "/api/v1/game/join";
const std::string GAME_PLAYERS = "/api/v1/game/players";

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

std::string UrlDecode(std::string_view str) {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
        char c = str[i];

        if (c == '%') {
            if (i + 2 < str.size()) {
                std::string hex = std::string(str.substr(i + 1, 2));
                char decoded_char = static_cast<char>(std::stoi(hex, nullptr, 16));
                result += decoded_char;
                i += 2;  
            }
        }
        else if (c == '+') {
            result += ' ';
        }
        else {
            result += c;
        }
    }

    return result;
}

bool IsSubPath(fs::path path, fs::path base) {
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    auto path_it = path.begin();
    auto base_it = base.begin();

    for (; base_it != base.end(); ++base_it, ++path_it) {
        if (path_it == path.end() || *path_it != *base_it) {
            return false;
        }
    }

    return true;
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
    if (ext == ".xml") return "application/xml";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpe" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".bmp") return "image/bmp";
    if (ext == ".ico") return "image/vnd.microsoft.icon";
    if (ext == ".tiff" || ext == ".tif") return "image/tiff";
    if (ext == ".svg" || ext == ".svgz") return "image/svg+xml";
    if (ext == ".mp3") return "audio/mpeg";

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

}
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
                    Send&& send)
    {
        const std::string target = std::string(req.target());
        const auto method = req.method();

        auto make_json = [&](http::status status,
                             std::string code,
                             std::string message)
        {
            http::response<http::string_body> res{status, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");

            json::object body{
                {"code", std::move(code)},
                {"message", std::move(message)}
            };

            res.body() = json::serialize(body);
            res.prepare_payload();
            return res;
        };

        auto send_json = [&](auto&& res) {
            send(std::move(res));
        };

        auto is_get_head = [&](http::verb m) {
            return m == http::verb::get || m == http::verb::head;
        };

        if (target == GAME_TICK) {
            if (tick_period_.has_value()) {
                send_json(make_json(http::status::bad_request,
                                    "badRequest",
                                    "Invalid endpoint"));
                return;
            }

            if (!is_get_head(method) && method != http::verb::post) {
                send_json(make_json(http::status::method_not_allowed,
                                    "invalidMethod",
                                    "Invalid method"));
                return;
            }

            api_handler_.HandleTick(std::move(req), std::move(send));
            return;
        }

        if (target == GAME_JOiN) {
            if (method != http::verb::post) {
                send_json(make_json(http::status::method_not_allowed,
                                    "invalidMethod",
                                    "Invalid method"));
                return;
            }

            api_handler_.HandleJoin(std::move(req), std::move(send));
            return;
        }

        if (target == GAME_PLAYERS) {
            if (!is_get_head(method)) {
                send_json(make_json(http::status::method_not_allowed,
                                    "invalidMethod",
                                    "Invalid method"));
                return;
            }

            api_handler_.HandlePlayers(std::move(req), std::move(send));
            return;
        }

        if (target == GAME_STATE) {
            if (!is_get_head(method)) {
                send_json(make_json(http::status::method_not_allowed,
                                    "invalidMethod",
                                    "Invalid method"));
                return;
            }

            api_handler_.HandleState(req, std::move(send));
            return;
        }

        if (target == GAME_ACTION) {
            if (method != http::verb::post) {
                send_json(make_json(http::status::method_not_allowed,
                                    "invalidMethod",
                                    "Invalid method"));
                return;
            }

            api_handler_.HandleAction(req, std::move(send));
            return;
        }

        if (target == MAPS_ENDPOINT) {
            if (!is_get_head(method)) {
                send_json(make_json(http::status::method_not_allowed,
                                    "invalidMethod",
                                    "Invalid method"));
                return;
            }

            json::array maps_array;

            for (const auto& map : game_.GetMaps()) {
                json::object map_obj;
                map_obj[json_keys::ID] = *map.GetId();
                map_obj[json_keys::NAME] = map.GetName();
                maps_array.push_back(std::move(map_obj));
            }

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");

            res.body() = json::serialize(maps_array);

            if (method == http::verb::head) {
                res.body() = {};
            }

            res.prepare_payload();
            send(std::move(res));
            return;
        }

        if (target.starts_with(MAP_ENDPOINT_PREFIX)) {

            if (!is_get_head(method)) {
                send_json(make_json(http::status::method_not_allowed,
                                    "invalidMethod",
                                    "Invalid method"));
                return;
            }

            std::string map_id = target.substr(MAP_ENDPOINT_PREFIX.size());

            const model::Map* map =
                game_.FindMap(model::Map::Id{map_id});

            if (!map) {
                send_json(make_json(http::status::not_found,
                                    "mapNotFound",
                                    "Map not found"));
                return;
            }

            json::array roads;
            for (const auto& r : map->GetRoads())
                roads.push_back(SerializeRoad(r));

            json::array buildings;
            for (const auto& b : map->GetBuildings())
                buildings.push_back(SerializeBuilding(b));

            json::array offices;
            for (const auto& o : map->GetOffices())
                offices.push_back(SerializeOffice(o));

            json::object result;
            result[json_keys::ID] = *map->GetId();
            result[json_keys::NAME] = map->GetName();
            result["roads"] = std::move(roads);
            result["buildings"] = std::move(buildings);
            result["offices"] = std::move(offices);

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");

            res.body() = json::serialize(result);

            if (method == http::verb::head) {
                res.body() = {};
            }

            res.prepare_payload();
            send(std::move(res));
            return;
        }

        {
            if (!is_get_head(method)) {
                send_json(make_json(http::status::method_not_allowed,
                                    "invalidMethod",
                                    "Invalid method"));
                return;
            }

            auto decoded = UrlDecode(target);
            fs::path file_path;

            if (decoded == "/") {
                file_path = static_root_ / "index.html";
            } else {
                file_path = static_root_ / decoded.substr(1);
            }

            if (fs::exists(file_path) && fs::is_directory(file_path)) {
                file_path /= "index.html";
            }

            if (!fs::exists(file_path)) {
                http::response<http::string_body> res{http::status::not_found, req.version()};
                res.set(http::field::content_type, "text/plain");
                res.set(http::field::cache_control, "no-cache");
                res.body() = "404 Not Found";
                res.prepare_payload();
                send(std::move(res));
                return;
            }

            file_path = fs::weakly_canonical(file_path);

            if (!IsSubPath(file_path, static_root_)) {
                send_json(make_json(http::status::bad_request,
                                    "badRequest",
                                    "Bad request"));
                return;
            }

            http::file_body::value_type file;
            beast::error_code ec;

            file.open(file_path.c_str(), beast::file_mode::read, ec);

            if (ec) {
                http::response<http::string_body> res{
                    http::status::internal_server_error, req.version()
                };
                res.set(http::field::content_type, "text/plain");
                res.set(http::field::cache_control, "no-cache");
                res.body() = "500 Internal Server Error";
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

            res.set(http::field::content_type, GetMimeType(file_path));
            res.set(http::field::cache_control, "no-cache");
            res.content_length(size);
            res.keep_alive(req.keep_alive());

            send(std::move(res));
            return;
        }
    }

private:
    model::Game& game_;
    model::PlayerManager& players_;
    std::filesystem::path static_root_;
    extra_data::Storage extra_data_;
    ApiHandler api_handler_;
    bool randomize_spawn_;
    std::optional<int> tick_period_;
};

} // namespace http_handler
