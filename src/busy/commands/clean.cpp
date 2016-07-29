#include "commands.h"

#include "NeoWorkspace.h"
#include <busyUtils/busyUtils.h>
#include <iostream>


using namespace busy;

namespace commands {

void clean() {
	NeoWorkspace ws;
	auto toolchain = ws.getSelectedToolchain();
	auto buildMode = ws.getSelectedBuildMode();

	auto cleanBusyPath  = std::string("./.busy/") + toolchain + "/" + buildMode + "/";
	auto cleanBuildPath = std::string("./build/") + toolchain + "/" + buildMode + "/";

	//!TODO reset lastCompileTime

	if (utils::fileExists(cleanBusyPath)) {
		utils::rm(cleanBusyPath, true);
	}
	if (utils::fileExists(cleanBuildPath)) {
		utils::rm(cleanBuildPath, true);
	}

	std::cout << "cleaned " << cleanBusyPath << " and " << cleanBuildPath << std::endl;

}

}
