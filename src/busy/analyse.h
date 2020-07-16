#pragma once

#include "CompilePipe.h"
#include "Package.h"


#include <filesystem>
#include <set>
#include <vector>


namespace busy {

// check if this _project is included by _allIncludes
auto isDependentProject(std::set<std::filesystem::path> const& _allIncludes, Project const& _project) -> bool;
auto findDependentProjects(Project const& _project, std::vector<Project> const& _projects) -> std::set<Project const*>;
auto createProjects(std::vector<Project> const& _projects) -> ProjectMap;
auto normalizeProjects(ProjectMap const& _projectMap) -> ProjectMap;
void checkConsistency(std::vector<Project> const& _projects);
auto listConsistencyIssues(std::vector<Project> const& _projects) -> std::vector<std::tuple<Project const*, Project const*>>;

}
