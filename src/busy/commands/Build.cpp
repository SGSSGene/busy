#include "Build.h"

#include <fstream>
#include <iostream>
#include "git.h"


namespace busy {
namespace commands {
namespace detail {

void Build::createAllNeededPaths() const {
	std::set<std::string> neededPath;
	for (auto const& project : ws.getProjectAndDependencies(rootProjectName)) {
		// paths for .o and .d files
		for (auto file : project->getCppFiles()) {
			neededPath.insert(utils::dirname(buildPath + "/" + file));
		}
		for (auto file : project->getCFiles()) {
			neededPath.insert(utils::dirname(buildPath + "/" + file));
		}

		// paths for .a files
		neededPath.insert(utils::dirname(buildPath + "/" + project->getFullName()));

		// paths needed for .a of singleFileProjects
		for (auto f : project->getCppAndCFiles()) {
			neededPath.insert(utils::dirname(buildPath + "/" + f));
		}

		// paths for executables
		std::string outputFile = outPath + "/" + project->getName();
		if (project->getIsUnitTest()) {
			outputFile = outPath + "/tests/" + project->getFullName();
		} else if (project->getIsExample()) {
			outputFile = outPath + "/examples/" + project->getFullName();
		}
		neededPath.insert(utils::dirname(outputFile));
	}

	for (auto const& s : neededPath) {
		if (not utils::fileExists(s)) {
			utils::mkdir(s);
		}
	}
}
void Build::createVersionFiles() const {
	utils::mkdir(".busy/helper-include/busy-version");
	std::ofstream versionFile(".busy/helper-include/busy-version/version.h");
	versionFile << "#pragma once" << std::endl;
	for (auto const& package : ws.getPackages()) {
	//	std::cout << package.getName() << " found at " << package.getPath() << std::endl;
		auto branch = std::string{"no-branch"};
		auto hash   = std::string{"00000000"};
		auto dirty  = bool(true);
		if (git::isGit(package.getPath())) {
			branch = git::getBranch(package.getPath());
			hash   = git::getCurrentHash(package.getPath());
			dirty  = git::isDirty(package.getPath(), true);
			hash = hash.substr(0, 8);
		}

		auto name   = package.getName();
		auto date   = utils::getDate();

		std::transform(name.begin(), name.end(), name.begin(), [](char c) { return std::toupper(c); });


		std::string versionName = "VERSION_" + name;
		std::string version     = hash;
		if (dirty) {
			version = version + "_dirty";
		}
		auto userName = git::getConfig(package.getPath(), "user.name");
		if (userName == "") {
			userName = "UnknownUser";
		}
		version = branch + "-" + version + " build on " + date + " by " + userName;

		versionFile << "#define " << versionName << " " << "\"" << version << "\"" << std::endl;
	}
}

auto Build::getStatisticUpdateCallback() const -> std::function<void(int, int)> {
	return [=] (int done, int total) {
		return updateStatisticUpdate(done, total);
	};
}
void Build::updateStatisticUpdate(int done, int total) const {
	std::lock_guard<std::mutex> lock(printMutex);
	if (errorDetected) return;
	if (console) {
		std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";
		std::cout << "working on job: " << done << "/" << total << std::flush;

		if (done == total) {
			std::cout << std::endl;
		}
	} else if (done == total) {
		std::cout << "working on job: "<< done << "/" << total << std::endl;
	}
}

}
}
}
