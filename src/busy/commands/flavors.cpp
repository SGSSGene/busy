#include "commands.h"

#include "Workspace.h"
#include <iostream>

#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"

using namespace busy;

namespace commands {

void flavors() {
	Workspace ws;
	auto flavors = ws.getFlavors();

	int longestString = 0;
	for (auto const& e : flavors) {
		longestString = std::max(longestString, int(e.first.length()));
	}

	for (auto const& e : flavors) {
		bool match = (e.second->buildMode == ws.getSelectedBuildMode()
			and e.second->toolchain == ws.getSelectedToolchain());

		if (match) {
			std::cout << TERM_GREEN;
		}
		std::cout << e.first;
		for (int i (e.first.length()); i < longestString; ++i) {
			std::cout << " ";
		}
		std::cout << std::endl;
		if (match) {
			std::cout << TERM_RESET;
		}

	}
}

}
