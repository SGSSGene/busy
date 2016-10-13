#include "commands.h"

#include "Workspace.h"
#include <iostream>

#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"

using namespace busy;

namespace commands {

void toolchains(bool _isTerminal) {
	Workspace ws;
	auto toolchains    = ws.getToolchains();

	std::string green = TERM_GREEN;
	std::string reset = TERM_RESET;
	if (not _isTerminal) {
		green = "";
		reset = "";
	}

	for (auto const& e : toolchains) {
		if (e.first == ws.getSelectedToolchain()) {
			std::cout << green;
		}
		std::cout << e.first;
		auto info = e.second->getAdditionalInfo();
		if (info != "" and _isTerminal) {
			std::cout << " (" << info << ")";
		}
		if (e.first == ws.getSelectedToolchain()) {
			std::cout << reset;
		}
		std::cout << "\n";
	}
}

}
