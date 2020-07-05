#pragma once

#include "CompilePipe.h"
#include "Package.h"


#include <filesystem>
#include <set>
#include <vector>


namespace busy::analyse {

// check if this _project is included by _allIncludes
auto isDependentProject(std::set<std::filesystem::path> const& _allIncludes, busy::analyse::Project const& _project) -> bool;
auto findDependentProjects(busy::analyse::Project const& _project, std::vector<busy::analyse::Project> const& _projects) -> std::set<busy::analyse::Project const*>;
auto createProjects(std::vector<busy::analyse::Project> const& _projects) -> ProjectMap;
auto normalizeProjects(ProjectMap const& _projectMap) -> ProjectMap;
void checkConsistency(std::vector<busy::analyse::Project> const& _projects);

}
