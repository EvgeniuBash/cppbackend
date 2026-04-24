#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <random>
#include <chrono>

#include "tagged.h"
#include "loot_generator.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

struct Position {
    double x = 0.0;
    double y = 0.0;
};


class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    double GetDogSpeed() const { return dog_speed_; }
    void SetDogSpeed(double speed) { dog_speed_ = speed; }

    void SetLootTypesCount(size_t count) {
        loot_types_count_ = count;
    }

    size_t GetLootTypesCount() const {
        return loot_types_count_;
    }

    void SetBagCapacity(size_t cap) { 
        bag_capacity_ = cap;
    }
    size_t GetBagCapacity() const {
        return bag_capacity_;
    }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    double dog_speed_ = 1.0;
    size_t loot_types_count_ = 0;
    size_t bag_capacity_ = 3;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

struct LostObject {
    int id;
    Map::Id map_id;
    size_t type;
    Position pos;
};

class Game {
public:

    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept {
        if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
            return &maps_.at(it->second);
        }
        return nullptr;
    }

    const std::unordered_map<int, LostObject>& GetLostObjects() const {
        return lost_objects_;
    }

    void RemoveLostObject(int id) {
        lost_objects_.erase(id);
    }

    void GenerateLoot(std::chrono::milliseconds delta, const Map& map, size_t player_count) {
        size_t current_loot = 0;

        for (const auto& [id, obj] : lost_objects_) {
            if (obj.map_id == map.GetId()) {
                ++current_loot;
            }
        }

        unsigned to_generate = loot_generator_.Generate(
            delta,
            static_cast<unsigned>(current_loot),
            static_cast<unsigned>(player_count)
        );

        if (to_generate == 0) return;

        if (map.GetRoads().empty()) return;

        static std::mt19937 gen(std::random_device{}());

        for (unsigned i = 0; i < to_generate; ++i) {
            const auto& roads = map.GetRoads();

            std::uniform_int_distribution<size_t> road_dist(0, roads.size() - 1);
            const auto& road = roads[road_dist(gen)];

            model::Position pos;

            if (road.IsHorizontal()) {
                std::uniform_real_distribution<double> x_dist(
                    std::min(road.GetStart().x, road.GetEnd().x),
                    std::max(road.GetStart().x, road.GetEnd().x)
                );

                pos.x = x_dist(gen);
                pos.y = road.GetStart().y;
            } else {
                std::uniform_real_distribution<double> y_dist(
                    std::min(road.GetStart().y, road.GetEnd().y),
                    std::max(road.GetStart().y, road.GetEnd().y)
                );

                pos.x = road.GetStart().x;
                pos.y = y_dist(gen);
            }

            std::uniform_int_distribution<size_t> type_dist(0, map.GetLootTypesCount() - 1);

            LostObject obj{
                next_loot_id_++,
                map.GetId(),
                type_dist(gen),
                pos
            };

            lost_objects_.emplace(obj.id, obj);
        }
    }
    
    void SetDogRetirementTime(double seconds) {
        dog_retirement_time_ = std::chrono::milliseconds{
            static_cast<int64_t>(seconds * 1000)
        };
    }

    std::chrono::milliseconds GetDogRetirementTime() const {
        return dog_retirement_time_;
    }
   

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    std::chrono::milliseconds dog_retirement_time_{60000};

    std::vector<Map> maps_;
    MapIdToIndex map_id_to_index_;
    std::unordered_map<int, LostObject> lost_objects_;
    int next_loot_id_ = 0;
    loot_gen::LootGenerator loot_generator_{
        std::chrono::milliseconds(1000), 
        0.5   
    };
};

}  // namespace model
