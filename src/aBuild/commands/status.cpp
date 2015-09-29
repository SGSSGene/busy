#include "commands.h"

using namespace aBuild;

namespace commands {


void status(std::string _flavor) {
	Workspace ws(".");
	if (_flavor != "") {
		if (_flavor == "release" || _flavor == "debug") {
			ws.accessConfigFile().setFlavor(_flavor);
		} else {
			throw "only \"release\" and \"debug\" are valid flavor arguments";
		}
	}
	std::cout << "current flavor:    " << ws.accessConfigFile().getFlavor() << std::endl;
	std::cout << "current toolchain: " << ws.accessConfigFile().getToolchain() << std::endl;
}

}
