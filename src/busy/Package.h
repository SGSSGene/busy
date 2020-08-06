#pragma once

#include "Project.h"

#include <filesystem>
#include <vector>

namespace busy {

auto readPackage(std::filesystem::path const& _workspaceRoot, std::filesystem::path const& _path) -> std::tuple<std::vector<Project>, std::vector<std::filesystem::path>>;



}
