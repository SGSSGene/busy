#include "commands.h"

#include "NeoWorkspace.h"
#include <busyUtils/busyUtils.h>
#include <iostream>


using namespace busy;

namespace commands {

void cleanAll() {
	utils::rm("./build", true);
	utils::mkdir("./build");

	//!TODO reset lastCompileTime

	auto dirs = utils::listDirs(".busy", true);
	for (auto d : dirs) {
		utils::rm(".busy/" + d, true);
	}

	auto files = utils::listFiles(".busy", false);
	for (auto f : files) {
		if (f != "workspace.bin") {
			utils::rm(".busy/" + f, false);
		}
	}
	std::cout << "clean all done" << std::endl;


}

}
