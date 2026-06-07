#include <catch2/catch_test_macros.hpp>
#include "model.h"

TEST_CASE("Loot is generated") {
    model::Game game;

    model::Map map{model::Map::Id{"map1"}, "Test"};

    map.AddRoad(model::Road(model::Road::HORIZONTAL, {0, 0}, 10));
    map.SetLootTypesCount(5);

    game.AddMap(map);

    size_t players = 1;

    game.GenerateLoot(std::chrono::milliseconds(5000), map, players);

    REQUIRE(game.GetLostObjects().size() <= 1);
}

TEST_CASE("Loot type in range") {
    model::Game game;

    model::Map map{model::Map::Id{"map1"}, "Test"};

    map.AddRoad(model::Road(model::Road::HORIZONTAL, {0, 0}, 10));
    map.SetLootTypesCount(3);

    game.AddMap(map);

    game.GenerateLoot(std::chrono::milliseconds(5000), map, 5);

    for (const auto& [id, obj] : game.GetLostObjects()) {
        REQUIRE(obj.type >= 0);
        REQUIRE(obj.type < 3);
    }
}

TEST_CASE("Loot spawned on road") {
    model::Game game;

    model::Map map{model::Map::Id{"map1"}, "Test"};

    map.AddRoad(model::Road(model::Road::HORIZONTAL, {0, 0}, 10));
    map.SetLootTypesCount(5);

    game.AddMap(map);

    game.GenerateLoot(std::chrono::milliseconds(5000), map, 2);

    for (const auto& [id, obj] : game.GetLostObjects()) {
        REQUIRE(obj.pos.y == 0.0);
        REQUIRE(obj.pos.x >= 0);
        REQUIRE(obj.pos.x <= 10);
    }
}