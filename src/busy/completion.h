#pragma once

#include <set>
#include <string>
#include <vector>

namespace busy::comp {

auto toolchain(std::vector<std::string> const& str) -> std::pair<bool, std::set<std::string>>;
auto options(std::vector<std::string> const& str) -> std::pair<bool, std::set<std::string>>;
auto projects(std::vector<std::string> const& str) -> std::pair<bool, std::set<std::string>>;


}
