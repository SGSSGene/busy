#pragma once

#include "TranslationSet.h"

#include <filesystem>
#include <vector>

namespace busy {

struct Package {
    // List of all translations sets (aka a list of all libraries/executables)
    std::vector<TranslationSet>         translationSets;
    std::vector<std::filesystem::path>  packages;
};

auto readPackage(std::filesystem::path _workspaceRoot, std::filesystem::path const& _path) -> Package;

constexpr std::string_view external{"external"};


}
