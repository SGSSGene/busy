#include "../FileCache.h"
#include "../cache.h"
#include "../config.h"
#include "../utils.h"

namespace busy::cmd {
namespace {

void showDeps() {
	auto workPath = std::filesystem::current_path();
	auto config   = loadConfig(workPath, *cfgBuildPath, {cfgBusyPath, *cfgBusyPath});

	auto [projects, packages] = busy::readPackage(config.rootDir, ".");

	packages.insert(begin(packages), user_sharedPath);
	packages.insert(begin(packages), global_sharedPath);

	// check consistency of packages
	fmt::print("checking consistency...");
	checkConsistency(projects);
	fmt::print("done\n");

	auto projects_with_deps = createProjects(projects);
	printProjects(projects_with_deps);
}

auto cmd = sargp::Command{"show-deps", "show dependencies of projects", showDeps};

}
}
