#pragma once

#include <filesystem>

#include "model.h"

namespace json_loader {

model::Game LoadGame(const std::filesystem::path& json_path, extra_data::Storage& extra_storage);

}  // namespace json_loader
