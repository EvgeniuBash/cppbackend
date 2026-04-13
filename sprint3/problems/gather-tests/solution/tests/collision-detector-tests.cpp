#define CATCH_CONFIG_MAIN
#include <catch2/catch_all.hpp>

#include "../src/collision_detector.h"

#include <cmath>
#include <sstream>

using namespace collision_detector;

namespace Catch {
template<>
struct StringMaker<GatheringEvent> {
    static std::string convert(GatheringEvent const& value) {
        std::ostringstream tmp;
        tmp << "(" << value.gatherer_id << "," << value.item_id
            << "," << value.sq_distance << "," << value.time << ")";
        return tmp.str();
    }
};
}

// ----------------------
// Тестовый провайдер
// ----------------------

class TestProvider : public ItemGathererProvider {
public:
    std::vector<Item> items;
    std::vector<Gatherer> gatherers;

    size_t ItemsCount() const override {
        return items.size();
    }

    Item GetItem(size_t idx) const override {
        return items.at(idx);
    }

    size_t GatherersCount() const override {
        return gatherers.size();
    }

    Gatherer GetGatherer(size_t idx) const override {
        return gatherers.at(idx);
    }
};

const double EPS = 1e-10;

// ----------------------
// ТЕСТЫ
// ----------------------

TEST_CASE("No movement -> no events") {
    TestProvider p;

    p.items = { {{0, 0}, 1.0} };
    p.gatherers = {
        {{0, 0}, {0, 0}, 1.0} // не двигается
    };

    auto events = FindGatherEvents(p);

    REQUIRE(events.empty());
}

TEST_CASE("Single collision") {
    TestProvider p;

    p.items = { {{5, 0}, 1.0} };
    p.gatherers = {
        {{0, 0}, {10, 0}, 1.0}
    };

    auto events = FindGatherEvents(p);

    REQUIRE(events.size() == 1);

    const auto& e = events[0];

    CHECK(e.item_id == 0);
    CHECK(e.gatherer_id == 0);
    CHECK(e.time == Approx(0.5).margin(EPS));
    CHECK(e.sq_distance == Approx(0.0).margin(EPS));
}

TEST_CASE("No collision (too far)") {
    TestProvider p;

    p.items = { {{5, 5}, 1.0} };
    p.gatherers = {
        {{0, 0}, {10, 0}, 1.0}
    };

    auto events = FindGatherEvents(p);

    REQUIRE(events.empty());
}

TEST_CASE("Multiple items collected in order") {
    TestProvider p;

    p.items = {
        {{2, 0}, 1.0},
        {{8, 0}, 1.0}
    };

    p.gatherers = {
        {{0, 0}, {10, 0}, 1.0}
    };

    auto events = FindGatherEvents(p);

    REQUIRE(events.size() == 2);

    CHECK(events[0].time < events[1].time);

    CHECK(events[0].item_id == 0);
    CHECK(events[1].item_id == 1);
}

TEST_CASE("Multiple gatherers") {
    TestProvider p;

    p.items = {
        {{5, 0}, 1.0}
    };

    p.gatherers = {
        {{0, 0}, {10, 0}, 1.0},
        {{10, 0}, {0, 0}, 1.0}
    };

    auto events = FindGatherEvents(p);

    REQUIRE(events.size() == 2);

    CHECK(events[0].time <= events[1].time);
}

TEST_CASE("Projection outside segment -> no collision") {
    TestProvider p;

    p.items = { {{15, 0}, 1.0} };
    p.gatherers = {
        {{0, 0}, {10, 0}, 1.0}
    };

    auto events = FindGatherEvents(p);

    REQUIRE(events.empty());
}

TEST_CASE("Correct distance computation") {
    TestProvider p;

    p.items = { {{5, 1}, 1.0} };
    p.gatherers = {
        {{0, 0}, {10, 0}, 1.0}
    };

    auto events = FindGatherEvents(p);

    REQUIRE(events.size() == 1);

    CHECK(events[0].sq_distance == Approx(1.0).margin(EPS));
}