#pragma once

#include "Project.h"

#include <filesystem>
#include <vector>

namespace busy {

auto readPackage(std::filesystem::path _workspaceRoot, std::filesystem::path const& _path) -> std::tuple<std::vector<Project>, std::vector<std::filesystem::path>>;

constexpr std::string_view external{"external"};


}
