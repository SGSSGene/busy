#include "commands.h"

#include "Workspace.h"
#include <iostream>

using namespace busy;

namespace commands {


void listProjects() {
	Workspace ws;
	std::map<std::string, int> matches;
	for (auto const& package : ws.getPackages()) {
		for (auto const& project : package.getProjects()) {
			auto name = project.getFullName();

			matches[name] += 1;
			auto pos = name.find("/");
			auto second = name.substr(pos+1);
			matches[second] += 1;
		}
	}
	for (auto const& e : matches) {
		if (e.second > 1) continue;
		std::cout << e.first << "\n";
	}
}

}
