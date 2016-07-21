#include "commands.h"
#include <iostream>


using namespace aBuild;

namespace commands {

void clean() {
	Workspace ws(".");
	auto toolchain = ws.accessConfigFile().getToolchain();
	auto buildMode = ws.accessConfigFile().getBuildMode();
	auto cleanAbuildPath = std::string("./.busy/") + toolchain + "/" + buildMode + "/";
	auto cleanBuildPath  = std::string("./build/") + toolchain + "/" + buildMode + "/";
	auto& config = ws.accessConfigFile();
	config.setLastCompileTime(0);
	config.accessAutoFileStates().clear();
	ws.save();

	if (utils::fileExists(cleanAbuildPath)) {
		utils::rm(cleanAbuildPath, true);
	}
	if (utils::fileExists(cleanBuildPath)) {
		utils::rm(cleanBuildPath, true);
	}

	std::cout << "cleaned " << cleanAbuildPath << " and " << cleanBuildPath << std::endl;

}

}
