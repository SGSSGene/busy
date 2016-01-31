#include "commands.h"

using namespace aBuild;

#include <iostream>
namespace commands {

void git(std::vector<std::string> const& _options) {
	std::vector<std::string> call;
	call.push_back("git");
	for (auto o : _options) {
		call.push_back(o);
	}

	Workspace ws(".");

	auto validPackages = ws.getAllValidPackages();
	for (auto const& p : validPackages) {
		process::Process proc(call, "packages/" + p.getName());
		std::cout << p.getName() << ":" << std::endl;
		if (proc.cout().size() > 0) {
			std::cout << proc.cout() << std::endl;
		}
		if (proc.cerr().size() > 0) {
			std::cerr << proc.cerr() << std::endl;
		}
	}

	process::Process proc(call, ".");
	std::cout << ws.getAllValidPackages(true).back().getName() << ":" << std::endl;
	if (proc.cout().size() > 0) {
		std::cout << proc.cout() << std::endl;
	}
	if (proc.cerr().size() > 0) {
		std::cerr << proc.cerr() << std::endl;
	}

}

}

