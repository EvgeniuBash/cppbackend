#pragma once

#include <filesystem>

#include "model.h"
#include "map_extra_data.h" 

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path, extra_data::Storage& extra_storage);

}  // namespace json_loader
