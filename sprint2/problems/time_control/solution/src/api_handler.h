#pragma once

#include "model.h"
#include <boost/json.hpp>
#include "http_server.h"
#include "player.h"
#include <string>

namespace http_handler {

namespace http = boost::beast::http;
namespace json = boost::json;

class ApiHandler {
public:
    ApiHandler(model::Game& game, model::PlayerManager& players)
        : game_(game), players_(players) {}

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

        auto& player = players_.AddPlayer(name, map->GetId());

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
        }

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");
        res.body() = "{}";
        res.prepare_payload();

        send(std::move(res));
    }

private:
void MovePlayerAlongRoad(model::Player* player, double dt) {
    if (!player || dt <= 0) return;

    const model::Map* map = game_.FindMap(player->GetMapId());
    if (!map) return;

    auto pos = player->GetPosition();
    auto speed = player->GetSpeed();
    auto dir = player->GetDirection();

    double new_x = pos.x + speed.vx * dt;
    double new_y = pos.y + speed.vy * dt;

    bool moved = false;

    // --- Движение по горизонтальной дороге ---
    if (speed.vx != 0) {
        for (const auto& road : map->GetRoads()) {
            if (!road.IsHorizontal()) continue;

            double yc = road.GetStart().y;
            double left  = std::min(road.GetStart().x, road.GetEnd().x);
            double right = std::max(road.GetStart().x, road.GetEnd().x);

            if (std::abs(pos.y - yc) <= 0.4) {
                // Ограничиваем по X границами дороги
                new_x = std::clamp(new_x, left - 0.4, right + 0.4);

                // Смещаем Y по направлению игрока
                double y_offset = (dir == model::Direction::NORTH) ? -0.4 :
                                  (dir == model::Direction::SOUTH) ? 0.4 : 0.0;

                player->SetPosition({new_x, yc + y_offset});
                moved = true;
                break;
            }
        }
    }
    // --- Движение по вертикальной дороге ---
    else if (speed.vy != 0) {
        for (const auto& road : map->GetRoads()) {
            if (!road.IsVertical()) continue;

            double xc = road.GetStart().x;
            double top    = std::min(road.GetStart().y, road.GetEnd().y);
            double bottom = std::max(road.GetStart().y, road.GetEnd().y);

            if (std::abs(pos.x - xc) <= 0.4) {
                // Ограничиваем по Y границами дороги
                new_y = std::clamp(new_y, top - 0.4, bottom + 0.4);

                // Смещаем X по направлению игрока
                double x_offset = (dir == model::Direction::WEST) ? -0.4 :
                                  (dir == model::Direction::EAST) ? 0.4 : 0.0;

                player->SetPosition({xc + x_offset, new_y});
                moved = true;
                break;
            }
        }
    }

    // --- Выравнивание по дороге (только перпендикулярно движению) ---
    if (moved) {
        pos = player->GetPosition();
        for (const auto& road : map->GetRoads()) {
            if (road.IsHorizontal() && pos.x >= std::min(road.GetStart().x, road.GetEnd().x) - 0.4 &&
                pos.x <= std::max(road.GetStart().x, road.GetEnd().x) + 0.4 &&
                std::abs(pos.y - road.GetStart().y) <= 0.4) {
                double y_offset = (dir == model::Direction::NORTH) ? -0.4 :
                                  (dir == model::Direction::SOUTH) ? 0.4 : 0.0;
                player->SetPosition({pos.x, road.GetStart().y + y_offset});
                return;
            }
            if (road.IsVertical() && pos.y >= std::min(road.GetStart().y, road.GetEnd().y) - 0.4 &&
                pos.y <= std::max(road.GetStart().y, road.GetEnd().y) + 0.4 &&
                std::abs(pos.x - road.GetStart().x) <= 0.4) {
                double x_offset = (dir == model::Direction::WEST) ? -0.4 :
                                  (dir == model::Direction::EAST) ? 0.4 : 0.0;
                player->SetPosition({road.GetStart().x + x_offset, pos.y});
                return;
            }
        }
    }
    // --- Если совсем не на дороге — обнуляем скорость ---
    else {
        player->SetSpeed({0, 0});
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
};

} // namespace http_handler