#include "commands.h"
#include <iostream>
#include <errno.h>


using namespace aBuild;

namespace commands {

void install() {
	Workspace ws(".");
	auto buildMode     = ws.accessConfigFile().getBuildMode();

	auto allToolchains = getAllToolchains(ws);

	Toolchain toolchain = allToolchains.rbegin()->second;
	std::string toolchainName = ws.accessConfigFile().getToolchain();
	if (allToolchains.find(toolchainName) != allToolchains.end()) {
		toolchain = allToolchains.at(toolchainName);
	}
	ws.accessConfigFile().setToolchain(toolchain.getName());
	ws.save();

	auto buildPath = "./build/" + toolchainName + "/" + buildMode + "/";

	auto allFiles = utils::listFiles(buildPath);

	for (auto const& f : allFiles) {

		auto oldFile = buildPath+f;
		auto newFile = std::string("/usr/bin/")+f;

		std::cout<<"installing "<<f<<"; Error Code: ";
		auto error = ::rename(oldFile.c_str(), newFile.c_str());
		switch(errno) {
		case EACCES:
			std::cout<<"EACCES"<<std::endl;
			break;
		case EBUSY:
			std::cout<<"EBUSY"<<std::endl;
			break;
		case ENOTEMPTY:
			std::cout<<"ENOTEMPTY"<<std::endl;
			break;
		case EINVAL:
			std::cout<<"EINVAL"<<std::endl;
			break;
		case EISDIR:
			std::cout<<"EISDIR"<<std::endl;
			break;
		case EMLINK:
			std::cout<<"EMLINK"<<std::endl;
			break;
		case ENOENT:
			std::cout<<"ENOENT"<<std::endl;
			break;
		case ENOSPC:
			std::cout<<"ENOSPC"<<std::endl;
			break;
		case EROFS:
			std::cout<<"EROFS"<<std::endl;
			break;
		case EXDEV: // files are on different partitions
			std::vector<std::string> cp;
			cp.push_back("cp");
			cp.push_back(oldFile);
			cp.push_back(newFile);
			process::Process p(cp);
			error = p.getStatus();
			break;
		}
		std::cout<<error<<std::endl;
	}
}

}
