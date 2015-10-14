#include "commands.h"

using namespace aBuild;

namespace commands {


void push() {
	auto allPackages = utils::listDirs("./packages", true);
	for (auto& p : allPackages) { p = "./packages/"+p; }
	allPackages.push_back(".");

	utils::runParallel<std::string>(allPackages, [](std::string const& path) {
		git::push(path);
	});
}

}
