#include "commands.h"

#include "Workspace.h"
#include <iostream>

using namespace busy;

namespace commands {


void listProjects() {
	Workspace ws;
	for (auto const& package : ws.getPackages()) {
		for (auto const& project : package.getProjects()) {
			std::cout << project.getFullName() << std::endl;
		}
	}
}

}
