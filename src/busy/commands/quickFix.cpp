#include "commands.h"

using namespace aBuild;

namespace commands {

void quickFix() {
	Package package {PackageURL()};

	if (utils::fileExists("busy.yaml")) {
		serializer::yaml::read("busy.yaml", package);
	} else {
		auto dir = utils::explode(utils::cwd(), "/");
		std::string packageName = dir[dir.size()-1];

		package.setName(packageName);
	}

	auto projectDirs = utils::listDirs("src", true);
	for (auto const& d : projectDirs) {
		bool found = false;
		for (auto const& p : package.accessProjects()) {
			if (p.getName() == d) {
				found = true;
				break;
			}
		}
		if (not found) {
			Project p;
			p.set(d);
			package.accessProjects().push_back(std::move(p));
		}
	}
	for (auto& project : package.accessProjects()) {
		project.quickFix();
	}
	serializer::yaml::write("busy.yaml", package);
}


}
