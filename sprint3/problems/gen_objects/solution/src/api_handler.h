#pragma once

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include "http_server.h"
#include <string>
#include <random>
#include <string_view>

#include "model.h"
#include "player.h"
#include "map_extra_data.h"

namespace http_handler {

namespace http = boost::beast::http;
namespace json = boost::json;

class ApiHandler {
public:
    ApiHandler(model::Game& game, model::PlayerManager& players, extra_data::Storage& extra_data, bool randomize_spawn)
        : game_(game)
        , players_(players)
        , extra_data_(extra_data)
        , randomize_spawn_(randomize_spawn) {}

    template <typename Send>
    void HandleJoin(const http::request<http::string_body>& req, Send&& send) {
        if (req.method() != http::verb::post) {
            sendMethodNotAllowed(req, send, "POST");
            return;
        }

        json::value body;
        try {
            body = json::parse(req.body());
        } catch (...) {
            sendBadRequest(req, send, "Join game request parse error");
            return;
        }

        auto obj = body.as_object();
        if (!obj.contains("userName") || !obj.contains("mapId")) {
            sendBadRequest(req, send, "Missing userName or mapId");
            return;
        }

        std::string name = obj["userName"].as_string().c_str();
        std::string map_id = obj["mapId"].as_string().c_str();

        if (name.empty()) {
            sendBadRequest(req, send, "Invalid name");
            return;
        }

        const model::Map* map = game_.FindMap(model::Map::Id{map_id});
            if (!map) {
                sendNotFound(req, send, "mapNotFound", "Map not found");
                return;
        }

        if (map->GetRoads().empty()) {
            sendBadRequest(req, send, "Map has no roads");
            return;
        }

        model::Position start_pos;

        if (randomize_spawn_) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_int_distribution<size_t> dist(0, map->GetRoads().size() - 1);
            const auto& road = map->GetRoads()[dist(gen)];
            start_pos.x = static_cast<double>(road.GetStart().x);
            start_pos.y = static_cast<double>(road.GetStart().y);
        } else {
            const auto& first_road = map->GetRoads().front();
            start_pos.x = static_cast<double>(first_road.GetStart().x);
            start_pos.y = static_cast<double>(first_road.GetStart().y);
        }

        auto& player = players_.AddPlayer(name, map->GetId(), start_pos);

        json::object resp{
            {"authToken", player.GetToken()},
            {"playerId", player.GetId()}
        };

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.body() = json::serialize(resp);
        res.prepare_payload();
        send(std::move(res));
    }

    template <typename Send>
    void HandlePlayers(const http::request<http::string_body>& req, Send&& send) {
        ExecuteAuthorized(req, send, [&](model::Player* player) {
            json::object players_json;
            for (auto* p : players_.GetPlayersByMap(player->GetMapId())) {
                players_json[std::to_string(p->GetId())] = {{"name", p->GetName()}};
            }

            json::object result;
            result["players"] = players_json;

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            if (req.method() != http::verb::head) {
                res.body() = json::serialize(result);
            }
            res.prepare_payload();
            send(std::move(res));
        });
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleState(http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
        if (req.method() != http::verb::get && req.method() != http::verb::head) {
            sendMethodNotAllowed(req, send, "GET, HEAD");
            return;
        }

        ExecuteAuthorized(req, send, [&](model::Player* player) {
            json::object players_json;

            for (auto* p : players_.GetPlayersByMap(player->GetMapId())) {
                json::object obj;

                obj["pos"] = json::array({p->GetPosition().x, p->GetPosition().y});
                obj["speed"] = json::array({p->GetSpeed().vx, p->GetSpeed().vy});
                obj["dir"] = DirToString(p->GetDirection());

                players_json[std::to_string(p->GetId())] = obj;
            }

            json::object loot_json;

            for (const auto& [id, obj] : game_.GetLostObjects()) {
                if (obj.map_id == player->GetMapId()) {
                    json::object loot_obj;

                    loot_obj["type"] = obj.type;
                    loot_obj["pos"] = json::array{obj.pos.x, obj.pos.y};

                    loot_json[std::to_string(id)] = loot_obj;
                }
            }

            result["lostObjects"] = loot_json;

            json::object result;
            result["players"] = players_json;

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");

            if (req.method() != http::verb::head) {
                res.body() = json::serialize(result);
            }

            res.prepare_payload();
            send(std::move(res));
        });
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleAction(http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
        if (req.method() != http::verb::post) {
            sendMethodNotAllowed(req, send, "POST");
            return;
        }

        if (req[http::field::content_type] != "application/json") {
            sendBadRequest(req, send, "Invalid content type");
            return;
        }

        ExecuteAuthorized(req, send, [&](model::Player* player) {
            json::value body;
            try {
                body = json::parse(req.body());
            } catch (...) {
                sendBadRequest(req, send, "Failed to parse action");
                return;
            }

            if (!body.is_object() || !body.as_object().contains("move")) {
                sendBadRequest(req, send, "Failed to parse action");
                return;
            }

            std::string move = body.at("move").as_string().c_str();

            if (move != "L" && move != "R" && move != "U" && move != "D" && move != "") {
                sendBadRequest(req, send, "Failed to parse action");
                return;
            }

            const model::Map* map = game_.FindMap(player->GetMapId());
            if (!map) {
                sendBadRequest(req, send, "Map not found");
                return;
            }

            double s = map->GetDogSpeed();

            if (move == "L") {
                player->SetSpeed({-s, 0});
                player->SetDirection(model::Direction::WEST);
            } else if (move == "R") {
                player->SetSpeed({s, 0});
                player->SetDirection(model::Direction::EAST);
            } else if (move == "U") {
                player->SetSpeed({0, -s});
                player->SetDirection(model::Direction::NORTH);
            } else if (move == "D") {
                player->SetSpeed({0, s});
                player->SetDirection(model::Direction::SOUTH);
            } else {
                player->SetSpeed({0, 0});
            }

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");
            res.set(http::field::cache_control, "no-cache");
            res.body() = "{}";
            res.prepare_payload();

            send(std::move(res));
        });
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleTick(http::request<Body, http::basic_fields<Allocator>>& req, Send&& send) {
        if (req.method() != http::verb::post) {
            sendMethodNotAllowed(req, send, "POST");
            return;
        }

        if (req[http::field::content_type] != "application/json") {
            sendInvalidArgument(req, send, "Invalid content type");
            return;
        }

        json::value body;
        try {
            body = json::parse(req.body());
        } catch (...) {
            sendInvalidArgument(req, send, "Failed to parse tick request JSON");
            return;
        }

        if (!body.is_object()) {
            sendInvalidArgument(req, send, "Failed to parse tick request JSON");
            return;
        }

        auto& obj = body.as_object();

        if (!obj.contains("timeDelta") || !obj.at("timeDelta").is_int64()) {
            sendInvalidArgument(req, send, "Invalid timeDelta");
            return;
        }

        int64_t delta_ms = obj.at("timeDelta").as_int64();
        if (delta_ms < 0) {
            sendInvalidArgument(req, send, "Invalid timeDelta");
            return;
        }

        double dt = static_cast<double>(delta_ms) / 1000.0;

        for (const auto& map : game_.GetMaps()) {
            auto players = players_.GetPlayersByMap(map.GetId());
            for (auto* player : players) {
                MovePlayerAlongRoad(player, dt);
            }

            game_.GenerateLoot(
                std::chrono::milliseconds(static_cast<int>(delta_ms)),
                map,
                players.size()
            ); 
        }
   

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.body() = "{}";
        res.prepare_payload();

        send(std::move(res));
    }

    template <typename Send>
    void HandleMapInfo(const http::request<http::string_body>& req, Send&& send, const std::string& map_id) {
        const model::Map* map = game_.FindMap(model::Map::Id{map_id});
        if (!map) {
            sendNotFound(req, send, "mapNotFound", "Map not found");
            return;
        }

        json::object result;

        const auto& extra = extra_data_.Get(map->GetId());
        result["lootTypes"] = extra;

        json::array roads;
        for (const auto& r : map->GetRoads()) {
            json::object obj;

            obj["x0"] = r.GetStart().x;
            obj["y0"] = r.GetStart().y;

            if (r.IsHorizontal())
                obj["x1"] = r.GetEnd().x;
            else
                obj["y1"] = r.GetEnd().y;

            roads.push_back(obj);
        }
        result["roads"] = roads;

        json::array buildings;
        for (const auto& b : map->GetBuildings()) {
            json::object obj;
            obj["x"] = b.GetBounds().position.x;
            obj["y"] = b.GetBounds().position.y;
            obj["w"] = b.GetBounds().size.width;
            obj["h"] = b.GetBounds().size.height;
            buildings.push_back(obj);
        }
        result["buildings"] = buildings;

        json::array offices;
        for (const auto& o : map->GetOffices()) {
            json::object obj;
            obj["id"] = std::string(o.GetId());
            obj["x"] = o.GetPosition().x;
            obj["y"] = o.GetPosition().y;
            obj["offsetX"] = o.GetOffset().dx;
            obj["offsetY"] = o.GetOffset().dy;
            offices.push_back(obj);
        }

        result["offices"] = offices;

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.body() = json::serialize(result);
        res.prepare_payload();

        send(std::move(res));
    }

private:
    void MovePlayerAlongRoad(model::Player* player, double dt) {
        if (!player || dt <= 0) {
            return;
        }

        const model::Map* map = game_.FindMap(player->GetMapId());
        if (!map) {
            return;
        }

        const auto pos = player->GetPosition();
        const auto speed = player->GetSpeed();

        const double target_x = pos.x + speed.vx * dt;
        const double target_y = pos.y + speed.vy * dt;

        bool found = false;
        model::Position best_pos = pos;
        model::Speed best_speed = speed;

        auto is_better = [&](const model::Position& cand) {
            if (!found) {
                return true;
            }
            if (speed.vx > 0) return cand.x > best_pos.x;
            if (speed.vx < 0) return cand.x < best_pos.x;
            if (speed.vy > 0) return cand.y > best_pos.y;
            if (speed.vy < 0) return cand.y < best_pos.y;
            return false;
        };

        for (const auto& road : map->GetRoads()) {
        double min_x, max_x, min_y, max_y;

        if (road.IsHorizontal()) {
            const double y = static_cast<double>(road.GetStart().y);
            const double left = static_cast<double>(std::min(road.GetStart().x, road.GetEnd().x));
            const double right = static_cast<double>(std::max(road.GetStart().x, road.GetEnd().x));

            min_x = left - 0.4;
            max_x = right + 0.4;
            min_y = y - 0.4;
            max_y = y + 0.4;
        } else {
            const double x = static_cast<double>(road.GetStart().x);
            const double top = static_cast<double>(std::min(road.GetStart().y, road.GetEnd().y));
            const double bottom = static_cast<double>(std::max(road.GetStart().y, road.GetEnd().y));

            min_x = x - 0.4;
            max_x = x + 0.4;
            min_y = top - 0.4;
            max_y = bottom + 0.4;
        }

        if (!(pos.x >= min_x && pos.x <= max_x &&
              pos.y >= min_y && pos.y <= max_y)) {
            continue;
        }

        double new_x = target_x;
        double new_y = target_y;
        model::Speed new_speed = speed;

        if (new_x < min_x) {
            new_x = min_x;
            new_speed.vx = 0.0;
        } else if (new_x > max_x) {
            new_x = max_x;
            new_speed.vx = 0.0;
        }

        if (new_y < min_y) {
            new_y = min_y;
            new_speed.vy = 0.0;
        } else if (new_y > max_y) {
            new_y = max_y;
            new_speed.vy = 0.0;
        }

        model::Position cand{new_x, new_y};

        if (is_better(cand)) {
            best_pos = cand;
            best_speed = new_speed;
            found = true;
            }
        }

        if (found) {
            player->SetPosition(best_pos);
            player->SetSpeed(best_speed);
        } else {
            player->SetSpeed({0.0, 0.0});
        }
    }
    
    template <typename Body, typename Allocator, typename Send, typename Fn>
    void ExecuteAuthorized(const http::request<Body, http::basic_fields<Allocator>>& req, Send&& send, Fn&& action) {
        auto it = req.find(http::field::authorization);
        if (it == req.end() || it->value().empty()) {
            sendUnauthorized(req, send, "invalidToken", "Authorization header missing");
            return;
        }

        std::string auth = std::string(it->value());
        const std::string prefix = "Bearer ";
        if (!auth.starts_with(prefix)) {
            sendUnauthorized(req, send, "invalidToken", "Authorization header invalid");
            return;
        }

        std::string token = auth.substr(prefix.size());

        auto isValidToken = [](const std::string& t) {
            if (t.size() != 32) return false;
            for (char c : t) if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
            return true;
        };

        if (!isValidToken(token)) {
            sendUnauthorized(req, send, "invalidToken", "Token invalid");
            return;
        }

        auto player = players_.FindByToken(token);
        if (!player) {
            sendUnauthorized(req, send, "unknownToken", "Token not found");
            return;
        }

        action(player);
    }

    template <typename Send>
    void sendBadRequest(const http::request<http::string_body>& req, Send&& send, const std::string& message) {
        sendError(req, send, http::status::bad_request, "invalidArgument", message);
    }

    template <typename Send>
    void sendNotFound(const http::request<http::string_body>& req, Send&& send, const std::string& code, const std::string& message) {
        sendError(req, send, http::status::not_found, code, message);
    }

    template <typename Send>
    void sendUnauthorized(const http::request<http::string_body>& req, Send&& send, const std::string& code, const std::string& message) {
        sendError(req, send, http::status::unauthorized, code, message);
    }

    template <typename Send>
    void sendMethodNotAllowed(const http::request<http::string_body>& req, Send&& send, const std::string& allow) {
        http::response<http::string_body> res{http::status::method_not_allowed, req.version()};
        res.set(http::field::allow, allow);
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");

        json::object body{
            {"code", "invalidMethod"},
            {"message", "Invalid method"}
        };
        res.body() = json::serialize(body);
        res.prepare_payload();
        send(std::move(res));
    }

    template <typename Send>
    void sendError(const http::request<http::string_body>& req, Send&& send, http::status status,
                   const std::string& code, const std::string& message) {
        http::response<http::string_body> res{status, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");

        json::object body{
            {"code", code},
            {"message", message}
        };
        res.body() = json::serialize(body);
        res.prepare_payload();
        send(std::move(res));
    }

    template <typename Send>
    void sendInvalidArgument(const http::request<http::string_body>& req,
                         Send&& send,
                         const std::string& message) {
        json::object obj;
        obj["code"] = "invalidArgument";
        obj["message"] = message;

        http::response<http::string_body> res{http::status::bad_request, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.body() = json::serialize(obj);
        res.prepare_payload();

        send(std::move(res));
    }

    static std::string DirToString(model::Direction dir) {
        switch (dir) {
            case model::Direction::NORTH: return "U";
            case model::Direction::SOUTH: return "D";
            case model::Direction::WEST:  return "L";
            case model::Direction::EAST:  return "R";
        }
        return "U";
    }

private:
    model::Game& game_;
    model::PlayerManager& players_;
    bool randomize_spawn_;
};

} // namespace http_handler