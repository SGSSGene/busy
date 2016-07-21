#include "commands.h"
#include <iostream>


using namespace aBuild;

namespace commands {

void cleanAll() {
	utils::rm("./build", true);
	utils::mkdir("./build");

	Workspace ws(".");
	auto& config = ws.accessConfigFile();

	config.setLastCompileTime(0);
	config.accessAutoFileStates().clear();
	ws.save();

	auto dirs = utils::listDirs(".busy", true);
	for (auto d : dirs) {
		utils::rm(".busy/" + d, true);
	}

	auto files = utils::listFiles(".busy", false);
	for (auto f : files) {
		if (f != "workspace.bin" && f != "workspace.yaml") {
			utils::rm(".busy/" + f, false);
		}
	}
	std::cout << "clean all done" << std::endl;


}

}
