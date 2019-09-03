#pragma once

#include "Project.h"

#include <filesystem>
#include <vector>

namespace busy::analyse {

auto readPackage(std::filesystem::path const& _root, std::filesystem::path const& _path) -> std::tuple<std::vector<Project>, std::vector<std::filesystem::path>>;



}
