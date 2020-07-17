#include "../FileCache.h"
#include "../cache.h"
#include "../config.h"
#include "../toolchains.h"
#include "../utils.h"

namespace busy::cmd {
namespace {

void lsToolchains() {
	auto workPath = std::filesystem::current_path();
	auto config   = loadConfig(workPath, *cfgBuildPath, {cfgRootPath, *cfgRootPath});

	auto packages = std::vector<std::filesystem::path>{};
	if (!config.rootDir.empty() and config.rootDir != "." and std::filesystem::exists(config.rootDir)) {
		auto [pro, pack] = busy::readPackage(config.rootDir, ".");
		for (auto const& p : pack) {
			packages.emplace_back(p);
		}
	}

	packages.insert(begin(packages), user_sharedPath);
	packages.insert(begin(packages), global_sharedPath);

	for (auto [name, path]  : searchForToolchains(packages)) {
		fmt::print("  - {} ({})\n", name, path);
	}
}

auto cmd = sargp::Command{"ls-toolchains", "list all available toolchains", lsToolchains};

}
}
