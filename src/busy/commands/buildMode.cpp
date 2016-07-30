#include "commands.h"

#include "Workspace.h"
#include <iostream>

#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"

using namespace busy;

namespace commands {

void buildMode(std::string const& _buildMode) {
	Workspace ws;

	ws.setSelectedBuildMode(_buildMode);
	std::cout << "current buildMode: " TERM_GREEN << ws.getSelectedBuildMode() << TERM_RESET << std::endl;
	std::cout << "current toolchain: " << ws.getSelectedToolchain() << std::endl;
}

}
