#include "commands.h"

using namespace aBuild;

namespace commands {

void install() {
	Workspace ws(".");
	auto flavor    = ws.accessConfigFile().getFlavor();

	auto allToolchains = getAllToolchains(ws);

	Toolchain toolchain = allToolchains.rbegin()->second;
	std::string toolchainName = ws.accessConfigFile().getToolchain();
	if (allToolchains.find(toolchainName) != allToolchains.end()) {
		toolchain = allToolchains.at(toolchainName);
	}
	ws.accessConfigFile().setToolchain(toolchain.getName());
	ws.save();

	auto buildPath = "./build/" + toolchainName + "/" + flavor + "/";

	auto allFiles = utils::listFiles(buildPath);

	for (auto const& f : allFiles) {

		auto oldFile = buildPath+f;
		auto newFile = std::string("/usr/bin/")+f;

		std::cout<<"installing "<<f<<"; Error Code: ";
		auto error = ::rename(oldFile.c_str(), newFile.c_str());
		std::cout<<error<<std::endl;
	}
}

}
