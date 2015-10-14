#include "commands.h"

using namespace aBuild;

namespace commands {

std::map<std::string, Toolchain> getAllToolchains(Workspace const& ws) {
	std::map<std::string, Toolchain> retList;

	std::map<std::string, Toolchain> searchPaths {
	     {"/usr/bin/gcc-4.9", {"system-gcc-4.9", true, "gcc-4.9", "g++-4.9", "ar"}},
	     {"/usr/bin/gcc-4.8", {"system-gcc-4.8", true, "gcc-4.8", "g++-4.8", "ar"}},
	     {"/usr/bin/gcc-4.7", {"system-gcc-4.7", true, "gcc-4.7", "g++-4.7", "ar"}},
	     {"/usr/bin/clang",   {"system-clang",   true, "clang",   "clang++", "ar"}},
	     };

	for (auto const& p : searchPaths) {
		if (utils::fileExists(p.first)) {
			retList[p.second.getName()] = p.second;
		}
	}

	for (auto const& package : ws.getAllValidPackages(true)) {
		for (auto const& tc : package.getToolchains()) {
			retList[tc.getName()] = tc;
		}
	}
	return retList;
}
std::map<std::string, Installation> getAllInstallations(Workspace const& ws) {
	std::map<std::string, Installation> retList;
	for (auto const& package : ws.getAllValidPackages(true)) {
		for (auto const& in : package.getInstallations()) {
			retList[in.getName()] = in;
		}
	}
	return retList;
}


void checkingMissingPackages(Workspace& ws) {
	// Checking missing packages
	auto missingPackages = ws.getAllMissingPackages();
	while (not missingPackages.empty()) {
		utils::runParallel<PackageURL>(missingPackages, [](PackageURL const& url) {
			utils::mkdir(".aBuild/tmp");
			std::string repoName = std::string(".aBuild/tmp/repo_") + url.getName() + ".git";
			utils::rm(repoName, true, true);
			std::cout << "cloning " << url.getURL() << std::endl;
			git::clone(".", url.getURL(), url.getBranch(), repoName);
			Package package(url);
			jsonSerializer::read(repoName + "/aBuild.json", package);
			utils::mv(repoName, std::string("packages/") + package.getName());
		});
		missingPackages = ws.getAllMissingPackages();
	}
}
void checkingNotNeededPackages(Workspace& ws) {
	// Checking not needed packages
	auto notRequiredPackages     = ws.getAllNotRequiredPackages();
	for (auto const& s : notRequiredPackages) {
		std::cout<<"Found not required package "<<s<<std::endl;
	}
}
void checkingInvalidPackages(Workspace& ws) {
	// checking invalid packages
	auto invalidPackages     = ws.getAllInvalidPackages();
	auto notRequiredPackages = ws.getAllNotRequiredPackages();
	for (auto const& p : invalidPackages) {
		if (estd::find(notRequiredPackages, p) == notRequiredPackages.end()) {
			std::cout<<"Package is ill formed: "<<p<<std::endl;
		}
	}
}

void checkingRequiredPackages(Workspace& ws) {
	// check branches (if the correct ones are checked out
	auto requiredPackages = ws.getAllRequiredPackages();
	for (auto const& _url : requiredPackages) {
		PackageURL url {_url};
		utils::Cwd cwd(url.getPath());
		if (not git::isDirty(url.getPath())) {
			if (url.getBranch() != git::getBranch(url.getPath())) {
				std::cout<<"Changing branch of "<<url.getName()<<std::endl;
				git::checkout(url.getPath(), url.getBranch());
			}
		}
	}
}



}
