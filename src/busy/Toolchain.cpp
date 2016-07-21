#include "Toolchain.h"

#include "Installation.h"

#include <iostream>

namespace busy {

bool Toolchain::isInstalled(std::map<std::string, Installation> const& installations) const {
	for (auto const& i : getInstallations()) {
		if (installations.find(i) == installations.end()) {
			return false;
		}
		if (not installations.at(i).isInstalled()) {
			return false;
		}
	}
	return true;
}
void Toolchain::install(std::map<std::string, Installation> const& installations) const {
	for (auto const& i : getInstallations()) {
		if (installations.find(i) == installations.end()) {
			std::cout << "unclear how to install " << i << std::endl;
			return;
		}
		if (not installations.at(i).isInstalled()) {
			installations.at(i).install();
		}

	}
}


}

