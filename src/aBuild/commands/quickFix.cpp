#include "commands.h"

using namespace aBuild;

namespace commands {

void quickFix() {
	if (not utils::fileExists("aBuild.json")) {
		auto dir = utils::explode(utils::cwd(), "/");
		std::string packageName = dir[dir.size()-1];

		Package package {PackageURL()};
		package.setName(packageName);

		auto projectDirs = utils::listDirs("src", true);
		for (auto const& d : projectDirs) {
			Project p;
			p.set(d, "executable");
			package.accessProjects().push_back(std::move(p));
		}
		serializer::json::write("aBuild.json", package);
	}
}


}
