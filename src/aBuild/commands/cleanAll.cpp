#include "commands.h"
#include <iostream>


using namespace aBuild;

namespace commands {

void cleanAll() {
	utils::rm("./build", true);
	utils::mkdir("./build");


	auto dirs = utils::listDirs(".aBuild", true);
	for (auto d : dirs) {
		utils::rm(".aBuild/" + d, true);
	}

	auto files = utils::listFiles(".aBuild", false);
	for (auto f : files) {
		if (f != "workspace.json" && f != "workspace.yaml") {
			utils::rm(".aBuild/" + f, false);
		}
	}
	std::cout << "clean all done" << std::endl;


}

}
