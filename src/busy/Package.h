#pragma once

#include "TranslationSet.h"

#include <filesystem>
#include <vector>

namespace busy {

auto readPackage(std::filesystem::path _workspaceRoot, std::filesystem::path const& _path) -> std::tuple<std::vector<TranslationSet>, std::vector<std::filesystem::path>>;

constexpr std::string_view external{"external"};


}
