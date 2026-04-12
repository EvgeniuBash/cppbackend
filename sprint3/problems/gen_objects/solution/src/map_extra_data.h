#pragma once
#include <boost/json.hpp>
#include <unordered_map>
#include "model.h"

namespace extra_data {

struct MapData {
    boost::json::array loot_types;
};

class Storage {
public:
    void Set(const model::Map::Id& id, boost::json::array loot) {
        data_[id] = MapData{std::move(loot)};
    }

    const boost::json::array& Get(const model::Map::Id& id) const {
        static boost::json::array empty;

        if (auto it = data_.find(id); it != data_.end()) {
            return it->second.loot_types;
        }
        return empty;
    }

private:
    std::unordered_map<model::Map::Id, boost::json::array, util::TaggedHasher> data_;
};

} // namespace extra_data