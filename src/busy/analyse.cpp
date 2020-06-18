#include "analyse.h"

namespace busy::analyse {

// check if this _project is included by _allIncludes
auto isDependentProject(std::set<std::filesystem::path> const& _allIncludes, busy::analyse::Project const& _project) -> bool {

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

auto findDependentProjects(busy::analyse::Project const& _project, std::vector<busy::analyse::Project> const& _projects) -> std::set<busy::analyse::Project const*> {
	auto ret = std::set<busy::analyse::Project const*>{};
	auto _allIncludes = _project.getIncludes();

	for (auto const& project : _projects) {
		if (isDependentProject(_allIncludes, project)) {
			ret.emplace(&project);
		}
	}
	return ret;
}

auto createProjects(std::vector<busy::analyse::Project> const& _projects) -> ProjectMap {
	using Project = busy::analyse::Project;

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

	return ret;
}

void checkConsistency(std::vector<busy::analyse::Project> const& _projects) {
	auto groupedProjects = std::map<std::string, std::vector<busy::analyse::Project const*>>{};
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

}