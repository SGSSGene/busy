#include "commands.h"

using namespace aBuild;

namespace commands {

std::map<std::string, Toolchain> getAllToolchains(Workspace const& ws) {
	std::map<std::string, Toolchain> retList;

	std::map<std::string, Toolchain> searchPaths {
	     {"/usr/bin/gcc",       {"system-gcc",       true, "gcc",       "g++",         "ar"}},
	     {"/usr/bin/gcc-5.3",   {"system-gcc-5.3",   true, "gcc-5.3",   "g++-5.3",     "ar"}},
	     {"/usr/bin/gcc-5.2",   {"system-gcc-5.2",   true, "gcc-5.2",   "g++-5.2",     "ar"}},
	     {"/usr/bin/gcc-5.1",   {"system-gcc-5.1",   true, "gcc-5.1",   "g++-5.1",     "ar"}},
	     {"/usr/bin/gcc-5.0",   {"system-gcc-5.0",   true, "gcc-5.0",   "g++-5.0",     "ar"}},
	     {"/usr/bin/gcc-4.9",   {"system-gcc-4.9",   true, "gcc-4.9",   "g++-4.9",     "ar"}},
	     {"/usr/bin/gcc-4.8",   {"system-gcc-4.8",   true, "gcc-4.8",   "g++-4.8",     "ar"}},
	     {"/usr/bin/gcc-4.7",   {"system-gcc-4.7",   true, "gcc-4.7",   "g++-4.7",     "ar"}},
	     {"/usr/bin/clang",     {"system-clang",     true, "clang",     "clang++",     "ar"}},
	     {"/usr/bin/clang-3.6", {"system-clang-3.6", true, "clang-3.6", "clang++-3.6", "ar"}},
	     {"/usr/bin/clang-3.8", {"system-clang-3.8", true, "clang-3.8", "clang++-3.8", "ar"}},
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

	std::set<PackageURL> queued;

	std::mutex mutex;
	threadPool::ThreadPool<PackageURL> threadPool;
	threadPool.spawnThread([&](PackageURL const& url) {
		utils::mkdir(".aBuild/tmp");
		std::string repoName = std::string(".aBuild/tmp/repo_") + url.getName() + ".git";
		utils::rm(repoName, true, true);
		{
			std::unique_lock<std::mutex> lock(mutex);
			std::cout << "cloning " << url.getURL() << std::endl;
		}
		git::clone(".", url.getURL(), url.getBranch(), repoName);
		Package package(url);
		serializer::json::read(repoName + "/aBuild.json", package);
		{
			std::unique_lock<std::mutex> lock(mutex);
			utils::mv(repoName, std::string("packages/") + package.getName());
			auto missingPackages = ws.getAllMissingPackages();
			for (auto m : missingPackages) {
				if (queued.count(m) == 0) {
					queued.insert(m);
					threadPool.queue(m);
				}
			}

		}
	}, 4);

	// Checking missing packages
	auto missingPackages = ws.getAllMissingPackages();
	queued.insert(missingPackages.begin(), missingPackages.end());
	threadPool.queueContainer(missingPackages);
	threadPool.wait();
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
		if (not git::isDirty(url.getPath())) {
			if (url.getBranch() != git::getBranch(url.getPath())) {
				std::cout << "Changing branch of " << url.getName();
				std::cout << " ( " << git::getBranch(url.getPath());
				std::cout << " -> " << url.getBranch() << " )" << std::endl;
				git::checkout(url.getPath(), url.getBranch());
			}
		}
	}
}



}
