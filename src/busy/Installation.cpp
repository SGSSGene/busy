#include "Installation.h"
#include <process/Process.h>

#include <iostream>

namespace busy {

System::System(busyConfig::InstallationSystem const& _system) {
	systems = _system.systems;
	type    = _system.type;
	url     = _system.url;
}

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

Installation::Installation(busyConfig::Installation const& _installation) {
	name = _installation.name;
	for (auto const& e : _installation.systems) {
		systems.emplace_back(e);
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
