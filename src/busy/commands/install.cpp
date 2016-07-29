#include "commands.h"

#include "NeoWorkspace.h"
#include <busyUtils/busyUtils.h>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <process/Process.h>


using namespace busy;

namespace commands {

void install() {
	NeoWorkspace ws;

	auto toolchainName = ws.getSelectedToolchain();
	auto buildModeName = ws.getSelectedBuildMode();

	auto buildPath = "./build/neo/" + toolchainName + "/" + buildModeName + "/";

	for (auto project : ws.getProjectAndDependencies()) {
		if (project->getType() != "executable") continue;
		if (project->getIsUnitTest()) continue;
		if (project->getIsExample()) continue;

		auto file = buildPath + project->getName();
		if (not utils::fileExists(file)) continue;

		auto oldFile = file;
		auto newFile = std::string("/usr/bin/")+project->getName();

		std::cout << "installing " << file << "; Error Code: ";
		auto error = ::rename(oldFile.c_str(), newFile.c_str());
		std::cout << error << " " << strerror(errno) << std::endl;

		if (errno == EXDEV) {
			std::cout << "trying to copy: " << std::endl;
			std::vector<std::string> cp;
			cp.push_back("cp");
			cp.push_back(oldFile);
			cp.push_back(newFile);
			process::Process p(cp);
			error = p.getStatus();
			std::cout << "error: " << error << (error?" Failed":"Successfull")<<std::endl;
			if (error != 0) {
				break;
			}
		}
	}
	return;
}

}
