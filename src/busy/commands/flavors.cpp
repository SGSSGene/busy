#include "commands.h"

#include "Workspace.h"
#include <iostream>

#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"

using namespace busy;

namespace commands {

void flavors(bool _isTerminal) {
	Workspace ws;
	auto flavors = ws.getFlavors();

	std::string green = TERM_GREEN;
	std::string reset = TERM_RESET;

	if (not _isTerminal) {
		green = reset = "";
	}

	std::map<std::string, int> matches;
	for (auto const& e : flavors) {
		matches[e.first] += 1;

		if (e.second->buildMode == ws.getSelectedBuildMode()
			and e.second->toolchain == ws.getSelectedToolchain()) {
			matches[e.first] = 0;
		}

		auto pos = e.first.find("/");
		auto second = e.first.substr(pos+1);
		matches[second] += 1;
	}

	for (auto const& e : matches) {
		if (e.second > 1) continue;

		bool match = e.second == 0;

		if (match) {
			std::cout << green;
		}
		std::cout << e.first;
		std::cout << std::endl;
		if (match) {
			std::cout << reset;
		}
	}
}
}
