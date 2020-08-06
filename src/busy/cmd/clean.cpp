#include <fmt/format.h>
#include <sargparse/sargparse.h>

#include "../FileCache.h"
#include "../cache.h"
#include "../config.h"
#include "../utils.h"

namespace busy::cmd {
namespace {

void clean() {
	auto workPath   = std::filesystem::current_path();
	auto config     = loadConfig(workPath, *cfgBuildPath, {cfgBusyPath, *cfgBusyPath});
	auto fileLock   = FileLock{};
	auto cacheGuard = loadFileCache(*cfgYamlCache);

	auto allRemovedFiles = std::uintmax_t{};
	for (auto& p : std::filesystem::directory_iterator{"."}) {
		if (p.path() != "./.busy.yaml" and p.path() != ".filecache") {
			allRemovedFiles += std::filesystem::remove_all(p.path());
		}
	}
	fmt::print("cleaned busy caches - removed {} files\n", allRemovedFiles);
}

auto cmd = sargp::Command{"clean", "cleans cache", clean};

}
}
