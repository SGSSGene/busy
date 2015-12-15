#include "commands.h"
#include <iostream>

using namespace aBuild;

namespace commands {


void status(std::string _flavor) {
	Workspace ws(".");
	if (_flavor != "") {
		if (_flavor == "release" || _flavor == "debug") {
			ws.accessConfigFile().setFlavor(_flavor);
		} else {
			throw "only \"release\" and \"debug\" are valid flavor arguments";
		}
	}

	auto allToolchains = getAllToolchains(ws);

	Toolchain toolchain = allToolchains.rbegin()->second;
	std::string toolchainName = ws.accessConfigFile().getToolchain();
	if (allToolchains.find(toolchainName) != allToolchains.end()) {
		toolchain = allToolchains.at(toolchainName);
	}
	ws.accessConfigFile().setToolchain(toolchain.getName());

	std::cout << "current flavor:    " << ws.accessConfigFile().getFlavor() << std::endl;
	std::cout << "current toolchain: " << ws.accessConfigFile().getToolchain() << std::endl;
}

}
