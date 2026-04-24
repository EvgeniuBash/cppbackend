#pragma once

#include <boost/beast/http.hpp>
#include <boost/json.hpp>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "collision_detector.h"
#include "http_server.h"
#include "logger.h"
#include "map_extra_data.h"
#include "model.h"
#include "player.h"
#include "records.h"

namespace http_handler {

namespace http = boost::beast::http;
namespace json = boost::json;

class ApiHandler {
public:
    ApiHandler(model::Game& game,
               model::PlayerManager& players,
               extra_data::Storage& extra_data,
               bool randomize_spawn,
               records::Repository& records_repo)
        : game_(game)
        , players_(players)
        , extra_data_(extra_data)
        , randomize_spawn_(randomize_spawn)
        , records_repo_(records_repo) {
    }

    template <typename Send>
    void HandleJoin(const http::request<http::string_body>& req, Send&& send) {
        if (req.method() != http::verb::post) {
            SendMethodNotAllowed(req, send, "POST");
            return;
        }

        if (!IsJsonContentType(req)) {
            SendInvalidArgument(req, send, "Invalid content type");
            return;
        }

        json::value body;
        try {
            body = json::parse(req.body());
        } catch (...) {
            SendInvalidArgument(req, send, "Join game request parse error");
            return;
        }

        if (!body.is_object()) {
            SendInvalidArgument(req, send, "Join game request parse error");
            return;
        }

        const auto& obj = body.as_object();

        if (!obj.contains("userName") || !obj.contains("mapId")) {
            SendInvalidArgument(req, send, "Join game request parse error");
            return;
        }

        if (!obj.at("userName").is_string() || !obj.at("mapId").is_string()) {
            SendInvalidArgument(req, send, "Join game request parse error");
            return;
        }

        std::string name = std::string(obj.at("userName").as_string());
        std::string map_id = std::string(obj.at("mapId").as_string());

        if (name.empty()) {
            SendInvalidArgument(req, send, "Invalid name");
            return;
        }

        const model::Map* map = game_.FindMap(model::Map::Id{map_id});
        if (!map) {
            SendNotFound(req, send, "mapNotFound", "Map not found");
            return;
        }

        if (map->GetRoads().empty()) {
            SendInvalidArgument(req, send, "Map has no roads");
            return;
        }

        model::Position start_pos = GetStartPosition(*map);

        auto& player = players_.AddPlayer(name, map->GetId(), start_pos);

        json::object resp{
            {"authToken", player.GetToken()},
            {"playerId", player.GetId()}
        };

        SendOkJson(req, send, resp);
    }

    template <typename Send>
    void HandlePlayers(const http::request<http::string_body>& req, Send&& send) {
        if (req.method() != http::verb::get &&
            req.method() != http::verb::head) {
            SendMethodNotAllowed(req, send, "GET, HEAD");
            return;
        }

        ExecuteAuthorized(req, send,
            [this, req, send](model::Player* player) mutable {
                json::object players_json;

                for (auto* p : players_.GetPlayersByMap(player->GetMapId())) {
                    players_json[std::to_string(p->GetId())] =
                        json::object{{"name", p->GetName()}};
                }

                json::object result;
                result["players"] = std::move(players_json);

                SendOkJson(req, send, result);
            });
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleState(const http::request<Body, http::basic_fields<Allocator>>& req,
                     Send&& send) {
        if (req.method() != http::verb::get &&
            req.method() != http::verb::head) {
            SendMethodNotAllowed(req, send, "GET, HEAD");
            return;
        }

        ExecuteAuthorized(req, send,
            [this, req, send](model::Player* player) mutable {
                json::object players_json;

                for (auto* p : players_.GetPlayersByMap(player->GetMapId())) {
                    json::array bag;

                    for (const auto& item : p->GetBag()) {
                        bag.push_back(json::object{
                            {"id", item.id},
                            {"type", item.type}
                        });
                    }

                    json::object player_obj;
                    player_obj["pos"] = json::array{
                        p->GetPosition().x,
                        p->GetPosition().y
                    };
                    player_obj["speed"] = json::array{
                        p->GetSpeed().vx,
                        p->GetSpeed().vy
                    };
                    player_obj["dir"] = DirToString(p->GetDirection());
                    player_obj["bag"] = std::move(bag);
                    player_obj["score"] = p->GetScore();

                    players_json[std::to_string(p->GetId())] = std::move(player_obj);
                }

                json::object loot_json;

                for (const auto& [id, obj] : game_.GetLostObjects()) {
                    if (obj.map_id == player->GetMapId()) {
                        loot_json[std::to_string(id)] = json::object{
                            {"type", obj.type},
                            {"pos", json::array{obj.pos.x, obj.pos.y}}
                        };
                    }
                }

                json::object result;
                result["players"] = std::move(players_json);
                result["lostObjects"] = std::move(loot_json);

                SendOkJson(req, send, result);
            });
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleAction(const http::request<Body, http::basic_fields<Allocator>>& req,
                      Send&& send) {
        if (req.method() != http::verb::post) {
            SendMethodNotAllowed(req, send, "POST");
            return;
        }

        if (!IsJsonContentType(req)) {
            SendInvalidArgument(req, send, "Invalid content type");
            return;
        }

        ExecuteAuthorized(req, send,
            [this, req, send](model::Player* player) mutable {
                json::value body;

                try {
                    body = json::parse(req.body());
                } catch (...) {
                    SendInvalidArgument(req, send, "Failed to parse action");
                    return;
                }

                if (!body.is_object()) {
                    SendInvalidArgument(req, send, "Failed to parse action");
                    return;
                }

                const auto& obj = body.as_object();

                if (!obj.contains("move") || !obj.at("move").is_string()) {
                    SendInvalidArgument(req, send, "Failed to parse action");
                    return;
                }

                const std::string move = std::string(obj.at("move").as_string());

                const model::Map* map = game_.FindMap(player->GetMapId());
                if (!map) {
                    SendInvalidArgument(req, send, "Map not found");
                    return;
                }

                const double s = map->GetDogSpeed();


                model::Speed new_speed{0.0, 0.0};

                if (move == "L") {
                    new_speed = {-s, 0.0};
                    player->SetDirection(model::Direction::WEST);
                } else if (move == "R") {
                    new_speed = {s, 0.0};
                    player->SetDirection(model::Direction::EAST);
                } else if (move == "U") {
                    new_speed = {0.0, -s};
                    player->SetDirection(model::Direction::NORTH);
                } else if (move == "D") {
                    new_speed = {0.0, s};
                    player->SetDirection(model::Direction::SOUTH);
                } else if (move == "") {
                    new_speed = {0.0, 0.0};
                } else {
                    SendInvalidArgument(req, send, "Invalid move");
                    return;
                }
                player->SetSpeed(new_speed);
                player->ResetIdleTime();

                SendOkJson(req, send, json::object{});
            });
    }

    template <typename Body, typename Allocator, typename Send>
    void HandleTick(const http::request<Body, http::basic_fields<Allocator>>& req,
                    Send&& send) {
        if (req.method() != http::verb::post) {
            SendMethodNotAllowed(req, send, "POST");
            return;
        }

        if (!IsJsonContentType(req)) {
            SendInvalidArgument(req, send, "Invalid content type");
            return;
        }

        json::value body;

        try {
            body = json::parse(req.body());
        } catch (...) {
            SendInvalidArgument(req, send, "Failed to parse tick request JSON");
            return;
        }

        if (!body.is_object()) {
            SendInvalidArgument(req, send, "Invalid tick request");
            return;
        }

        const auto& obj = body.as_object();

        if (!obj.contains("timeDelta") || !obj.at("timeDelta").is_int64()) {
            SendInvalidArgument(req, send, "Invalid timeDelta");
            return;
        }

        const int64_t delta_ms = obj.at("timeDelta").as_int64();

        if (delta_ms < 0) {
            SendInvalidArgument(req, send, "Invalid timeDelta");
            return;
        }

        ProcessTick(std::chrono::milliseconds{delta_ms});

        SendOkJson(req, send, json::object{});
    }
  
    void ProcessTick(std::chrono::milliseconds delta) {
        if (delta.count() < 0) {
            return;
        }

        RetireIdlePlayers(delta);

        const double dt = std::chrono::duration<double>(delta).count();

        for (const auto& map : game_.GetMaps()) {
            auto players_on_map = players_.GetPlayersByMap(map.GetId());

            for (auto* player : players_on_map) {
                player->SetPrevPosition(player->GetPosition());
                MovePlayerAlongRoad(player, dt);
            }

            CollectLoot(map, players_on_map);
            DeliverLootToOffices(map, players_on_map);

            game_.GenerateLoot(delta, map, players_on_map.size());
        }
    }

    template <typename Send>
    void HandleMapInfo(const http::request<http::string_body>& req,
                       Send&& send,
                       const std::string& map_id) {
        if (req.method() != http::verb::get &&
            req.method() != http::verb::head) {
            SendMethodNotAllowed(req, send, "GET, HEAD");
            return;
        }

        const model::Map* map = game_.FindMap(model::Map::Id{map_id});
        if (!map) {
            SendNotFound(req, send, "mapNotFound", "Map not found");
            return;
        }

        json::object result;
        result["lootTypes"] = extra_data_.Get(map->GetId());

        json::array roads;
        for (const auto& r : map->GetRoads()) {
            json::object obj;

            obj["x0"] = r.GetStart().x;
            obj["y0"] = r.GetStart().y;

            if (r.IsHorizontal()) {
                obj["x1"] = r.GetEnd().x;
            } else {
                obj["y1"] = r.GetEnd().y;
            }

            roads.push_back(std::move(obj));
        }

        json::array buildings;
        for (const auto& b : map->GetBuildings()) {
            const auto& bounds = b.GetBounds();

            buildings.push_back(json::object{
                {"x", bounds.position.x},
                {"y", bounds.position.y},
                {"w", bounds.size.width},
                {"h", bounds.size.height}
            });
        }

        json::array offices;
        for (const auto& office : map->GetOffices()) {
            const auto& pos = office.GetPosition();
            const auto& offset = office.GetOffset();

            offices.push_back(json::object{
                {"id", *office.GetId()},
                {"x", pos.x},
                {"y", pos.y},
                {"offsetX", offset.dx},
                {"offsetY", offset.dy}
            });
        }

        result["roads"] = std::move(roads);
        result["buildings"] = std::move(buildings);
        result["offices"] = std::move(offices);

        SendOkJson(req, send, result);
    }

private:
    static constexpr double ROAD_HALF_WIDTH = 0.4;
    static constexpr double DOG_WIDTH = 0.6;
    static constexpr double LOOT_WIDTH = 0.0;
    static constexpr double OFFICE_WIDTH = 0.5;

    model::Position GetStartPosition(const model::Map& map) const {
        if (!randomize_spawn_) {
            const auto& road = map.GetRoads().front();
            return {
                static_cast<double>(road.GetStart().x),
                static_cast<double>(road.GetStart().y)
            };
        }

        static std::random_device rd;
        static std::mt19937 gen(rd());

        std::uniform_int_distribution<size_t> road_dist(0, map.GetRoads().size() - 1);
        const auto& road = map.GetRoads()[road_dist(gen)];

        if (road.IsHorizontal()) {
            const int x0 = std::min(road.GetStart().x, road.GetEnd().x);
            const int x1 = std::max(road.GetStart().x, road.GetEnd().x);

            std::uniform_real_distribution<double> x_dist(
                static_cast<double>(x0),
                static_cast<double>(x1)
            );

            return {
                x_dist(gen),
                static_cast<double>(road.GetStart().y)
            };
        }

        const int y0 = std::min(road.GetStart().y, road.GetEnd().y);
        const int y1 = std::max(road.GetStart().y, road.GetEnd().y);

        std::uniform_real_distribution<double> y_dist(
            static_cast<double>(y0),
            static_cast<double>(y1)
        );

        return {
            static_cast<double>(road.GetStart().x),
            y_dist(gen)
        };
    }

    void MovePlayerAlongRoad(model::Player* player, double dt) {
        if (!player || dt <= 0.0) {
            return;
        }

        const model::Map* map = game_.FindMap(player->GetMapId());
        if (!map) {
            return;
        }

        const auto pos = player->GetPosition();
        const auto speed = player->GetSpeed();

        if (speed.vx == 0.0 && speed.vy == 0.0) {
            return;
        }

        const double target_x = pos.x + speed.vx * dt;
        const double target_y = pos.y + speed.vy * dt;

        bool found = false;
        model::Position best_pos = pos;
        model::Speed best_speed = speed;

        auto is_better = [&](const model::Position& cand) {
            if (!found) {
                return true;
            }

            if (speed.vx > 0.0) {
                return cand.x > best_pos.x;
            }

            if (speed.vx < 0.0) {
                return cand.x < best_pos.x;
            }

            if (speed.vy > 0.0) {
                return cand.y > best_pos.y;
            }

            if (speed.vy < 0.0) {
                return cand.y < best_pos.y;
            }

            return false;
        };

        for (const auto& road : map->GetRoads()) {
            double min_x = 0.0;
            double max_x = 0.0;
            double min_y = 0.0;
            double max_y = 0.0;

            if (road.IsHorizontal()) {
                const double y = static_cast<double>(road.GetStart().y);
                const double left = static_cast<double>(
                    std::min(road.GetStart().x, road.GetEnd().x)
                );
                const double right = static_cast<double>(
                    std::max(road.GetStart().x, road.GetEnd().x)
                );

                min_x = left - ROAD_HALF_WIDTH;
                max_x = right + ROAD_HALF_WIDTH;
                min_y = y - ROAD_HALF_WIDTH;
                max_y = y + ROAD_HALF_WIDTH;
            } else {
                const double x = static_cast<double>(road.GetStart().x);
                const double top = static_cast<double>(
                    std::min(road.GetStart().y, road.GetEnd().y)
                );
                const double bottom = static_cast<double>(
                    std::max(road.GetStart().y, road.GetEnd().y)
                );

                min_x = x - ROAD_HALF_WIDTH;
                max_x = x + ROAD_HALF_WIDTH;
                min_y = top - ROAD_HALF_WIDTH;
                max_y = bottom + ROAD_HALF_WIDTH;
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

            model::Position candidate{new_x, new_y};

            if (is_better(candidate)) {
                best_pos = candidate;
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

    void CollectLoot(const model::Map& map, const std::vector<model::Player*>& players) {
        std::vector<model::LostObject> items;

        for (const auto& [id, obj] : game_.GetLostObjects()) {
            if (obj.map_id == map.GetId()) {
                items.push_back(obj);
            }
        }

        if (items.empty() || players.empty()) {
            return;
        }

        class Provider final : public collision_detector::ItemGathererProvider {
        public:
            Provider(const std::vector<model::Player*>& players,
                     const std::vector<model::LostObject>& items)
                : players_(players)
                , items_(items) {
            }

            size_t ItemsCount() const override {
                return items_.size();
            }

            collision_detector::Item GetItem(size_t idx) const override {
                const auto& obj = items_[idx];

                return {
                    {obj.pos.x, obj.pos.y},
                    LOOT_WIDTH
                };
            }

            size_t GatherersCount() const override {
                return players_.size();
            }

            collision_detector::Gatherer GetGatherer(size_t idx) const override {
                const auto* player = players_[idx];

                return {
                    {player->GetPrevPosition().x, player->GetPrevPosition().y},
                    {player->GetPosition().x, player->GetPosition().y},
                    DOG_WIDTH
                };
            }

        private:
            const std::vector<model::Player*>& players_;
            const std::vector<model::LostObject>& items_;
        };

        Provider provider(players, items);
        auto events = collision_detector::FindGatherEvents(provider);

        std::unordered_set<int> collected_ids;

        for (const auto& event : events) {
            auto* player = players[event.gatherer_id];
            const auto& item = items[event.item_id];

            if (collected_ids.contains(item.id)) {
                continue;
            }

            if (player->GetBagSize() >= map.GetBagCapacity()) {
                continue;
            }

            player->AddToBag({item.id, item.type});
            game_.RemoveLostObject(item.id);

            collected_ids.insert(item.id);
        }
    }

    void DeliverLootToOffices(const model::Map& map,
                              const std::vector<model::Player*>& players) {
        if (players.empty() || map.GetOffices().empty()) {
            return;
        }

        for (auto* player : players) {
            if (player->GetBag().empty()) {
                continue;
            }

            bool near_office = false;

            for (const auto& office : map.GetOffices()) {
                const auto& pos = office.GetPosition();

                const double dx = player->GetPosition().x - pos.x;
                const double dy = player->GetPosition().y - pos.y;

                const double radius = DOG_WIDTH + OFFICE_WIDTH;

                if (dx * dx + dy * dy <= radius * radius) {
                    near_office = true;
                    break;
                }
            }

            if (!near_office) {
                continue;
            }

            DeliverBag(*player, map);
        }
    }

    void DeliverBag(model::Player& player, const model::Map& map) {
        const auto loot_types = extra_data_.Get(map.GetId());

        int total_score = 0;

        for (const auto& item : player.GetBag()) {
            if (item.type < 0 ||
                static_cast<size_t>(item.type) >= loot_types.size()) {
                continue;
            }

            const auto& loot_value = loot_types[item.type];

            if (!loot_value.is_object()) {
                continue;
            }

            const auto& loot_obj = loot_value.as_object();

            if (!loot_obj.contains("value") || !loot_obj.at("value").is_int64()) {
                continue;
            }

            total_score += static_cast<int>(loot_obj.at("value").as_int64());
        }

        if (total_score > 0) {
            player.AddScore(total_score);
        }

        player.ClearBag();
    }

    void RetireIdlePlayers(std::chrono::milliseconds delta) {
        std::vector<model::PlayerId> retired_players;
        std::vector<records::Record> records_to_save;

        const double retirement_seconds =
            std::chrono::duration<double>(game_.GetDogRetirementTime()).count();

        for (auto* player : players_.GetAllPlayers()) {
            player->Tick(delta);

            const double idle_seconds = player->GetIdleTime();

            if (idle_seconds >= retirement_seconds) {
                records_to_save.push_back({
                    player->GetName(),
                    player->GetScore(),
                    std::chrono::duration<double>(player->GetPlayTime()).count()
                });

                retired_players.push_back(player->GetId());
            }
        }

        if (!records_to_save.empty()) {
            try {
                records_repo_.SaveMany(records_to_save);
            } catch (const std::exception& e) {
                std::cerr << "SaveMany failed: " << e.what() << std::endl;
            }
        }

        for (auto id : retired_players) {
            try {
                players_.RemovePlayer(id);
            } catch (const std::exception& e) {
                std::cerr << "RemovePlayer failed: " << e.what() << std::endl;
            }
        }
    }

    template <typename Body, typename Fields>
    static bool IsJsonContentType(const http::request<Body, Fields>& req) {
        auto it = req.find(http::field::content_type);

        if (it == req.end()) {
            return false;
        }

        const std::string content_type = std::string(it->value());

        return content_type == "application/json" ||
               content_type.rfind("application/json;", 0) == 0;
    }

    template <typename Body, typename Fields, typename Send>
    void SendOkJson(const http::request<Body, Fields>& req,
                    Send&& send,
                    const json::value& body) {
        http::response<http::string_body> res{http::status::ok, req.version()};

        res.set(http::field::content_type, "application/json");
        res.set(http::field::cache_control, "no-cache");

        if (req.method() != http::verb::head) {
            res.body() = json::serialize(body);
        }

        res.prepare_payload();

        send(std::move(res));
    }

    template <typename Body, typename Fields, typename Send, typename Fn>
    void ExecuteAuthorized(const http::request<Body, Fields>& req,
                           Send&& send,
                           Fn&& action) {
        auto it = req.find(http::field::authorization);

        if (it == req.end() || it->value().empty()) {
            SendUnauthorized(req, send, "invalidToken", "Authorization missing");
            return;
        }

        const std::string auth = std::string(it->value());
        const std::string prefix = "Bearer ";

        if (auth.rfind(prefix, 0) != 0) {
            SendUnauthorized(req, send, "invalidToken", "Bad auth header");
            return;
        }

        const std::string token = auth.substr(prefix.size());

        if (!IsValidToken(token)) {
            SendUnauthorized(req, send, "invalidToken", "Invalid token");
            return;
        }

        auto* player = players_.FindByToken(token);

        if (!player) {
            SendUnauthorized(req, send, "unknownToken", "Player token has not been found");
            return;
        }

        action(player);
    }

    static bool IsValidToken(const std::string& token) {
        if (token.size() != 32) {
            return false;
        }

        for (char c : token) {
            if (!std::isxdigit(static_cast<unsigned char>(c))) {
                return false;
            }
        }

        return true;
    }

    template <typename Body, typename Fields, typename Send>
    void SendInvalidArgument(const http::request<Body, Fields>& req,
                             Send&& send,
                             const std::string& message) {
        SendError(req, send, http::status::bad_request, "invalidArgument", message);
    }

    template <typename Body, typename Fields, typename Send>
    void SendNotFound(const http::request<Body, Fields>& req,
                      Send&& send,
                      const std::string& code,
                      const std::string& message) {
        SendError(req, send, http::status::not_found, code, message);
    }

    template <typename Body, typename Fields, typename Send>
    void SendUnauthorized(const http::request<Body, Fields>& req,
                          Send&& send,
                          const std::string& code,
                          const std::string& message) {
        SendError(req, send, http::status::unauthorized, code, message);
    }

    template <typename Body, typename Fields, typename Send>
    void SendMethodNotAllowed(const http::request<Body, Fields>& req,
                              Send&& send,
                              const std::string& allow) {
        http::response<http::string_body> res{
            http::status::method_not_allowed,
            req.version()
        };

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

    template <typename Body, typename Fields, typename Send>
    void SendError(const http::request<Body, Fields>& req,
                   Send&& send,
                   http::status status,
                   const std::string& code,
                   const std::string& message) {
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

    static std::string DirToString(model::Direction dir) {
        switch (dir) {
            case model::Direction::NORTH:
                return "U";
            case model::Direction::SOUTH:
                return "D";
            case model::Direction::WEST:
                return "L";
            case model::Direction::EAST:
                return "R";
        }

        return "U";
    }

private:
    model::Game& game_;
    model::PlayerManager& players_;
    extra_data::Storage& extra_data_;
    bool randomize_spawn_;
    records::Repository& records_repo_;
};

} // namespace http_handler