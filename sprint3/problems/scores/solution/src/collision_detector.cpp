#include "collision_detector.h"
#include <cassert>

namespace collision_detector {

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {

    assert(b.x != a.x || b.y != a.y);
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    const double proj_ratio = u_dot_v / v_len2;
    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

    return CollectionResult(sq_distance, proj_ratio);
}

std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
    std::vector<GatheringEvent> events;

    const size_t items_count = provider.ItemsCount();
    const size_t gatherers_count = provider.GatherersCount();

    for (size_t gi = 0; gi < gatherers_count; ++gi) {
        const auto gatherer = provider.GetGatherer(gi);

        if (gatherer.start_pos.x == gatherer.end_pos.x &&
            gatherer.start_pos.y == gatherer.end_pos.y) {
            continue;
        }

        for (size_t ii = 0; ii < items_count; ++ii) {
            const auto item = provider.GetItem(ii);

            auto result = TryCollectPoint(
                gatherer.start_pos,
                gatherer.end_pos,
                item.position
            );

            double collect_radius = gatherer.width + item.width;

            if (!result.IsCollected(collect_radius)) {
                continue;
            }

            events.push_back({
                ii,                 // item_id
                gi,                 // gatherer_id
                result.sq_distance, // sq_distance
                result.proj_ratio  // time
            });
        }
    }

    std::sort(events.begin(), events.end(),
        [](const GatheringEvent& a, const GatheringEvent& b) {
            return a.time < b.time;
        });

    return events;
}

}  // namespace collision_detector