#include "commands.h"
#include <iostream>


using namespace busy;

namespace commands {

void clean() {
	Workspace ws(".");
	auto toolchain = ws.accessConfigFile().getToolchain();
	auto buildMode = ws.accessConfigFile().getBuildMode();
	auto cleanBusyPath  = std::string("./.busy/") + toolchain + "/" + buildMode + "/";
	auto cleanBuildPath = std::string("./build/") + toolchain + "/" + buildMode + "/";
	auto& config = ws.accessConfigFile();
	config.setLastCompileTime(0);
	config.accessAutoFileStates().clear();
	ws.save();

	if (utils::fileExists(cleanBusyPath)) {
		utils::rm(cleanBusyPath, true);
	}
	if (utils::fileExists(cleanBuildPath)) {
		utils::rm(cleanBuildPath, true);
	}

	std::cout << "cleaned " << cleanBusyPath << " and " << cleanBuildPath << std::endl;

}

}
