#include "commands.h"

#include "NeoWorkspace.h"
#include <iostream>

using namespace busy;

namespace commands {


void toolchains() {
	NeoWorkspace ws;
	auto toolchains    = ws.getToolchains();

	int longestString = 0;
	for (auto const& e : toolchains) {
		longestString = std::max(longestString, int(e.first.length()));
	}

	for (auto const& e : toolchains) {
		std::cout << e.first;
		for (int i (e.first.length()); i < longestString; ++i) {
			std::cout << " ";
		}
		std::cout << " (auto-generated)";
		std::cout << std::endl;
	}
}

}
