#include "commands.h"
#include <iostream>

using namespace busy;

namespace commands {

static void runTest(std::string const& call) {
	std::cout << " • running " << call << "...";
	process::Process p({call});
	if (p.getStatus() == 0) {
		std::cout << " no errors" << std::endl;
	} else {
		std::cout << " errors: " << std::endl;
		if (p.cout().size() > 0) {
			std::cout << p.cout() << std::endl;
		}
		if (p.cerr().size() > 0) {
			std::cerr << p.cerr() << std::endl;
		}
	}
}
void test() {
	Workspace ws(".");
	auto toolchain = ws.accessConfigFile().getToolchain();
	auto buildMode = ws.accessConfigFile().getBuildMode();
	auto buildPath = std::string("./build/") + toolchain + "/" + buildMode + "/";
	std::cout<<"===Start testing==="<<std::endl;
	if (utils::dirExists(buildPath + "tests/")) {
		auto allTests = utils::listFiles(buildPath + "tests/");
		for (auto const& t : allTests) {
			auto call = buildPath + "tests/"+t;
			runTest(call);
		}
	}
	for (auto const& d : utils::listDirs(buildPath, true)) {
		if (d == "tests") continue;
		std::string path = buildPath + d + "/tests/";
		if (not utils::dirExists(path)) continue;
		for (auto const& t : utils::listFiles(path)) {
			auto call = path+t;
			runTest(call);
		}
	}
	std::cout<<"===Ended testing==="<<std::endl;
}

}
