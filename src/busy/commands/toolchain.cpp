#include "commands.h"
#include <iostream>

using namespace busy;

namespace commands {

void toolchain(std::string const& _toolchain) {
	Workspace ws(".");
	auto toolchains    = getAllToolchains(ws);
	auto installations = getAllInstallations(ws);

	if (toolchains.find(_toolchain) != toolchains.end()) {
		if (not toolchains.at(_toolchain).isInstalled(installations)) {
			std::cout << "Toolchain is not installed." << std::endl;
			std::cout << "trying to install..." << std::endl;
			toolchains.at(_toolchain).install(installations);
			if (not toolchains.at(_toolchain).isInstalled(installations)) {
				return;
			}

		}
		ws.accessConfigFile().setToolchain(_toolchain);
		std::cout << "Set toolchain to " << _toolchain << std::endl;
		return;
	}
	std::cout << "Setting toolchain failed" << std::endl;
}

}
