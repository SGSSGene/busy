#include "commands.h"

using namespace aBuild;

namespace commands {

void quickFix() {
	Package package {PackageURL()};

	if (not utils::fileExists("aBuild.json")) {
		auto dir = utils::explode(utils::cwd(), "/");
		std::string packageName = dir[dir.size()-1];

		package.setName(packageName);
	} else {
		serializer::json::read("aBuild.json", package);
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
	serializer::json::write("aBuild.json", package);
}


}
