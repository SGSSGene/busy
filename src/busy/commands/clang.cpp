#include "commands.h"
#include <busyUtils/busyUtils.h>
#include <fstream>
#include <iostream>

#include "NeoWorkspace.h"

using namespace busy;

namespace commands {

void clang() {
	NeoWorkspace ws;
	std::ofstream ofs(".clang_complete");
	NeoVisitor visitor(ws, "");
	visitor.setProjectVisitor([&ofs] (NeoProject const* _project) {
		std::vector<std::string> options;
		options.push_back("-std=c++11");
		options.push_back("-DBUSY");
		options.push_back("-DBUSY_" + utils::sanitizeForMakro(_project->getName()));
		for (auto depP : _project->getDependencies()) {
			options.push_back("-DBUSY_" + utils::sanitizeForMakro(depP->getName()));
		}
		for (auto path : _project->getIncludeAndDependendPaths()) {
			options.push_back("-I " + path);
		}
		for (auto path : _project->getSystemIncludeAndDependendPaths()) {
			options.push_back("-isystem " + path);
		}
		for (auto const& o : options) {
			ofs << o << " ";
		}
		ofs << std::endl;
	});

	visitor.visit(1);
	std::cout << "generated .clang_complete file" << std::endl;
}


}
