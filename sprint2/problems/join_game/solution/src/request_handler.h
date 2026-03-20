#pragma once

#include <boost/json.hpp>
#include "http_server.h"
#include "model.h"
#include "api_handler.h"
#include <string>
#include <string_view>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

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
    explicit RequestHandler(model::Game& game, std::filesystem::path static_root)
        : game_(game) 
        , static_root_(std::move(static_root))
        , api_handler_(game_, players_)
        {}

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
            return;

        } else {
                auto decoded_target = UrlDecode(target);

                fs::path file_path;

                if (decoded_target == "/") {
                    file_path = static_root_ / "index.html";
                } else {
                    file_path = static_root_ / decoded_target.substr(1);
                }
                if (fs::exists(file_path) && fs::is_directory(file_path)) {
                    file_path /= "index.html";
                }
                if (!fs::exists(file_path)) {
                    http::response<http::string_body> response{
                    http::status::not_found, req.version()};

                    response.set(http::field::content_type, "text/plain");
                    response.body() = "404 Not Found";
                    response.prepare_payload();

                    send(std::move(response));
                    return;
                }

                file_path = fs::weakly_canonical(file_path);

                if (!IsSubPath(file_path, static_root_)) {
                    http::response<http::string_body> response{
                    http::status::bad_request, req.version()};

                    response.set(http::field::content_type, "text/plain");
                    response.body() = "400 Bad request";
                    response.prepare_payload();

                    send(std::move(response));
                    return;
                }
        
            http::file_body::value_type file;

            beast::error_code ec;

            file.open(file_path.c_str(), beast::file_mode::read, ec);

            if (ec) {
                http::response<http::string_body> response{
                http::status::internal_server_error, req.version()};

                response.set(http::field::content_type, "text/plain");
                response.body() = "500 Internal Server Error";
                response.prepare_payload();

                send(std::move(response));
                return;
             }

             auto const size = file.size();

             http::response<http::file_body> response{
             std::piecewise_construct,
             std::make_tuple(std::move(file)),
             std::make_tuple(http::status::ok, req.version())
         };

         response.set(http::field::content_type, GetMimeType(file_path));
         response.content_length(size);
         response.keep_alive(req.keep_alive());

         send(std::move(response));
         return;    
         }
    }
private:
    model::Game& game_;
    std::filesystem::path static_root_;
    ApiHandler api_handler_;
};

} // namespace http_handler
