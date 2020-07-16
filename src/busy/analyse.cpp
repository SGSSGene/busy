#include "analyse.h"


namespace busy {

// check if this _project is included by _allIncludes
auto isDependentProject(std::set<std::filesystem::path> const& _allIncludes, Project const& _project) -> bool {

	auto files = _project.getFiles();
	for (auto const& file : files) {
		// check if is includable by default path
		{
			auto path = _project.getName() / relative(file.getPath(), _project.getPath());
			if (_allIncludes.count(path)) {
				return true;
			}
		}
		// check if it is includable by legacy include path
		{
			for (auto const& p : _project.getLegacyIncludePaths()) {
				auto path = relative(file.getPath(), p);
				if (_allIncludes.count(path)) {
					return true;
				}
			}
		}
	}
	return false;
}

auto findDependentProjects(Project const& _project, std::vector<Project> const& _projects) -> std::set<Project const*> {
	auto ret = std::set<Project const*>{};
	auto _allIncludes = _project.getIncludes();

	for (auto const& project : _projects) {
		if (isDependentProject(_allIncludes, project)) {
			ret.emplace(&project);
		}
	}
	return ret;
}

auto createProjects(std::vector<Project> const& _projects) -> ProjectMap {
	auto ret = ProjectMap{};

	for (auto const& p : _projects) {
		ret[&p];
		auto deps = findDependentProjects(p, _projects);
		for (auto const& d : deps) {
			if (d == &p) continue;
			std::get<0>(ret[&p]).insert(d);
			std::get<1>(ret[d]).insert(&p);
		}
	}

	return normalizeProjects(ret);
}
auto normalizeProjects(ProjectMap const& _projectMap) -> ProjectMap {
	auto duplicateList = std::map<std::string, Project const*>{};
	auto ret = ProjectMap{};
	for (auto [key, deps] : _projectMap) {
		auto iter = duplicateList.find(key->getName());
		if (iter == end(duplicateList)) {
			duplicateList[key->getName()] = key;
			ret[key] = deps;
		}
	}
	for (auto& [key, deps] : ret) {
		auto ingoing  = std::get<0>(deps);
		auto outgoing = std::get<1>(deps);
		deps = {};

		for (auto v : ingoing) {
			std::get<0>(deps).insert(duplicateList.at(v->getName()));
		}
		for (auto v : outgoing) {
			std::get<1>(deps).insert(duplicateList.at(v->getName()));
		}
	}
	return ret;
}


void checkConsistency(std::vector<Project> const& _projects) {
	auto groupedProjects = std::map<std::string, std::vector<Project const*>>{};
	for (auto const& p : _projects) {
		groupedProjects[p.getName()].emplace_back(&p);
	}


	for (auto const& [name, list] : groupedProjects) {
		//!TODO this can be done much more efficient
		if (size(list) > 1) {
			for (auto const& p1 : list) {
				for (auto const& p2 : list) {
					if (p1 == p2) continue;
					if (not p1->isEquivalent(*p2)) {
						throw std::runtime_error("two projects don't seem to be the same: " + p1->getName());
					}
				}
			}
		}
	}
}

auto listConsistencyIssues(std::vector<Project> const& _projects) -> std::vector<std::tuple<Project const*, Project const*>> {
	auto groupedProjects = std::map<std::string, std::vector<Project const*>>{};
	for (auto const& p : _projects) {
		groupedProjects[p.getName()].emplace_back(&p);
	}

	auto retList = std::vector<std::tuple<Project const*, Project const*>>{};
	for (auto const& [name, list] : groupedProjects) {
		if (size(list) > 1) {
			for (auto const& p1 : list) {
				for (auto const& p2 : list) {
					if (p1 == p2) continue;
					if (not p1->isEquivalent(*p2)) {
						retList.emplace_back(p1, p2);
					}
				}
			}
		}
	}
	return retList;
}

}
