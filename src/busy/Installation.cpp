#include "Installation.h"
#include <process/Process.h>

#include <iostream>

namespace aBuild {
bool System::isInstalled() const {
	if (type == "system") {
		if (systems.count("ubuntu") > 0) {
			process::Process p({"which", url});
			return p.getStatus() == 0;
		}
	}
	return false;
}

void System::install() const {
	if (type == "system") {
		if (systems.count("ubuntu") > 0) {
			std::cout << "sudo apt-get install -y " << url << std::endl;
			process::Process p({"sudo", "apt-get", "install", "-y", url});
		}
	}
}


bool Installation::isInstalled() const {
	for (auto const& s : systems) {
		if (s.isInstalled()) {
			return true;
		}
	}
	return false;
}

void Installation::install() const {
	for (auto const& s : systems) {
		if (not s.isInstalled()) {
			s.install();
		}
	}
}


}
