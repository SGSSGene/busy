#include <sargparse/ArgumentParsing.h>
#include <sargparse/File.h>
#include <sargparse/Parameter.h>

#include <fmt/format.h>

namespace busy::cmd {
namespace {

void clean() {
	auto allRemovedFiles = std::uintmax_t{};
	for (auto& p : std::filesystem::directory_iterator{"."}) {
		if (p.path() != "./.busy.yaml") {
			allRemovedFiles += std::filesystem::remove_all(p.path());
		}
	}
	fmt::print("cleaned busy caches - removed {} files\n", allRemovedFiles);
}

auto cmd = sargp::Command{"clean", "cleans cache", &clean};

}
}
