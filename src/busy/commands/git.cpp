#include "commands.h"

#include "Workspace.h"
#include <iostream>
#include <process/Process.h>

namespace commands {
using namespace busy;

void git(std::vector<std::string> const& _options) {
	std::vector<std::string> call;
	call.push_back("git");
	for (auto o : _options) {
		call.push_back(o);
	}

	Workspace ws;
	auto validPackages = ws.getPackages();
	for (auto const& p : validPackages) {
		std::cout << "processing " << p.getName() << std::endl;
		process::InteractiveProcess proc(call, "extRepositories/" + p.getName());
	}

}

}

