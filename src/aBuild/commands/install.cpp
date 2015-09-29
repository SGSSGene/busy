#include "commands.h"

using namespace aBuild;

namespace commands {

void install() {
	Workspace ws(".");
	auto flavor    = ws.accessConfigFile().getFlavor();
	auto toolchain = ws.accessConfigFile().getToolchain();
	auto buildPath = "./build/" + toolchain + "/" + flavor + "/";

	auto allFiles = utils::listFiles(buildPath);

	for (auto const& f : allFiles) {
		std::cout<<"installing "<<f<<"; Error Code: ";
		auto oldFile = buildPath+f;
		auto newFile = std::string("/usr/bin/")+f;
		auto error = ::rename(oldFile.c_str(), newFile.c_str());
		std::cout<<error<<std::endl;
	}
}

}
