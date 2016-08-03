#include "commands.h"

#include "Workspace.h"
#include <busyUtils/busyUtils.h>
#include <iostream>


using namespace busy;

namespace commands {

void cleanAll() {
	utils::rm("./build", true);
	utils::mkdir("./build");
	utils::rm(".busy", true, true);
	std::cout << "clean all done" << std::endl;


}

}
