#include "commands.h"


using namespace aBuild;

namespace commands {

void clean() {
	Workspace ws(".");
	auto toolchain = ws.accessConfigFile().getToolchain();
	auto flavor    = ws.accessConfigFile().getFlavor();
	auto cleanAbuildPath = std::string("./.aBuild/") + toolchain + "/" + flavor + "/";
	auto cleanBuildPath  = std::string("./build/") + toolchain + "/" + flavor + "/";

	utils::rm(cleanAbuildPath, true);
	utils::rm(cleanBuildPath, true);

	std::cout << "cleaned " << cleanAbuildPath << " and " << cleanBuildPath << std::endl;

}

}
