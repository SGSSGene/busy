#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <vector>

namespace busy {

auto searchForToolchains(std::vector<std::filesystem::path> const& _paths) -> std::map<std::string, std::filesystem::path>;

auto getToolchainOptions(std::string_view _name, std::filesystem::path const& _path) -> std::map<std::string, std::vector<std::string>>;


}
