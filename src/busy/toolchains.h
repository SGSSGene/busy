#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <vector>

namespace busy {

auto searchForToolchains(std::filesystem::path _path) -> std::vector<std::tuple<std::string, std::filesystem::path>>;
auto searchForToolchains(std::vector<std::filesystem::path> const& _paths) -> std::map<std::string, std::filesystem::path>;

auto getToolchainOptions(std::string_view _name, std::filesystem::path _path) -> std::set<std::string>;


}
