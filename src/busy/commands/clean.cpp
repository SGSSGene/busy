#include "commands.h"

#include "Workspace.h"
#include <busyUtils/busyUtils.h>
#include <iostream>

#define TERM_RED                        "\033[31m"
#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"

using namespace busy;

namespace commands {

void clean() {
	Workspace ws;
	auto toolchain = ws.getSelectedToolchain();
	auto buildMode = ws.getSelectedBuildMode();

	auto cleanBusyPath  = std::string("./.busy/") + toolchain + "/" + buildMode + "/";
	auto cleanBuildPath = std::string("./build/") + toolchain + "/" + buildMode + "/";

	//!TODO reset lastCompileTime

	if (utils::fileExists(cleanBusyPath)) {
		utils::rm(cleanBusyPath, true);
	}
	if (utils::fileExists(cleanBuildPath)) {
		utils::rm(cleanBuildPath, true);
	}

	std::cout << "cleaned " << cleanBusyPath << " and " << cleanBuildPath << std::endl;

	auto validPackages = ws.getPackages();
	auto allDepPackages = validPackages.begin()->getAllDependendPackages();
	std::vector<Package const*> noDepPackage;
	for (auto const& p : validPackages) {
		auto iter = std::find_if(allDepPackages.begin(), allDepPackages.end(), [&](Package const* p2) {
			return p.getName() == p2->getName();
		});
		if (iter == allDepPackages.end()) {
			noDepPackage.push_back(&p);
		}
	}

	if (noDepPackage.size() > 0) {
		std::cout << "\n" << TERM_RED << "Packages is not used or referenced:\n" << TERM_RESET;
		std::string paths;
		for (auto p : noDepPackage) {
			std::cout << " - " << p->getName() << "\n";
			paths += " " + p->getPath();
		}
		std::cout << "you might want to run\n";
		std::cout << TERM_GREEN"  rm -rf " << paths << TERM_RESET"\n";
	}



}

}
