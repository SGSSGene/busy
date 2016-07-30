#include "commands.h"

#include "Workspace.h"
#include <iostream>

#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"

using namespace busy;

namespace commands {

void toolchains() {
	Workspace ws;
	auto toolchains    = ws.getToolchains();

	int longestString = 0;
	for (auto const& e : toolchains) {
		longestString = std::max(longestString, int(e.first.length()));
	}

	for (auto const& e : toolchains) {
		if (e.first == ws.getSelectedToolchain()) {
			std::cout << TERM_GREEN;
		}
		std::cout << e.first;
		for (int i (e.first.length()); i < longestString; ++i) {
			std::cout << " ";
		}
		std::cout << " (auto-generated)";
		std::cout << std::endl;
		if (e.first == ws.getSelectedToolchain()) {
			std::cout << TERM_RESET;
		}

	}
}

}
