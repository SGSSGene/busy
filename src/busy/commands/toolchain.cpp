#include "commands.h"

#include "Workspace.h"
#include <iostream>

#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"

using namespace busy;

namespace commands {

void toolchain(std::string const& _toolchain) {
	Workspace ws;

	ws.setSelectedToolchain(_toolchain);
	std::cout << "current buildMode: " << ws.getSelectedBuildMode() << std::endl;
	std::cout << "current toolchain: " TERM_GREEN << ws.getSelectedToolchain() << TERM_RESET << std::endl;
}

}
