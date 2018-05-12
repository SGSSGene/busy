#include "commands.h"

#include "Workspace.h"
#include <busyUtils/busyUtils.h>
#include <iostream>
#include <process/Process.h>

using namespace busy;

namespace commands {

void listBuildModes() {
	Workspace ws;
	for (auto const& x : ws.getBuildModes()) {
		std::cout << x << "\n";
	}
}
}
