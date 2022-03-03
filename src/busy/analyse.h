#pragma once

#include "CompilePipe.h"
#include "Package.h"


#include <filesystem>
#include <set>
#include <vector>


namespace busy {

// check if this _project is included by _allIncludes
auto isDependentTranslationSet(std::set<std::filesystem::path> const& _allIncludes, TranslationSet const& _project) -> bool;
auto findDependentTranslationSets(TranslationSet const& _project, std::vector<TranslationSet> const& _projects) -> std::set<TranslationSet const*>;
auto createTranslationSets(std::vector<TranslationSet> const& _projects) -> TranslationSetMap;
auto normalizeTranslationSets(TranslationSetMap const& _projectMap) -> TranslationSetMap;
void checkConsistency(std::vector<TranslationSet> const& _projects);
auto listConsistencyIssues(std::vector<TranslationSet> const& _projects) -> std::vector<std::tuple<TranslationSet const*, TranslationSet const*>>;

}
