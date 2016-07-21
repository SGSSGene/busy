#include "Workspace.h"
#include <iostream>

#include <busyUtils/busyUtils.h>
#include "estd.h"

#include <iostream>

namespace {
	const std::string workspaceFile { ".busy/workspace.bin" };
}


namespace busy {

Workspace::Workspace(std::string const& _path)
	: path {_path + "/"} {

	if (utils::fileExists(path + workspaceFile)) {
		serializer::binary::read(path + workspaceFile, configFile);
	}
	createBusyFolder();
	createPackageFolder();
}
Workspace::~Workspace() {
	save();
}

void Workspace::save() {
	std::string outFile = path + workspaceFile;
	utils::AtomicWrite atomic(outFile);
	serializer::binary::write(atomic.getTempName(), configFile);
	atomic.close();
}

auto Workspace::getAllMissingPackages() const -> std::vector<PackageURL> {
	std::set<PackageURL> urlSet;

	auto requiredPackages = getAllRequiredPackages();
	auto allPackages      = utils::listDirs(path + "extRepositories", true);

	for (auto const& r : requiredPackages) {
		bool found {false};
		for (auto const& p : allPackages) {
			if (p == r.getName()) {
				found = true;
				break;
			}
		}
		if (not found) {
			urlSet.insert(r);
		}
	}
	std::vector<PackageURL> retList;
	for (auto const& p : urlSet) {
		retList.push_back(p);
	}
	return retList;
}
auto Workspace::getAllValidPackages() const -> std::vector<Package> {
	std::vector<Package> retList;
	// Reading external repository directory finding available repositories
	// results is something like {"CommonOptions", "Process", "SelfTest"}
	auto allPackages = utils::listDirs(path + "extRepositories", true);

	auto p = readPackage(".");
	retList.emplace_back(std::move(p));

	for (auto const& package : allPackages) {
		try {
			auto packagePath = path + "extRepositories/" + package;
			auto p = readPackage(packagePath);
			retList.emplace_back(std::move(p));
		} catch(...) {}
	}

	return retList;
}
auto Workspace::getAllInvalidPackages() const -> std::vector<std::string> {
	std::vector<std::string> retList;
	// Reading repository directory finding available repositories
	auto allPackages = utils::listDirs(path + "extRepositories", true);
	for (auto const& s : allPackages) {
		try {
			std::string packagePath = path + "extRepositories/" + s;
			readPackage(packagePath);
		} catch (...) {
			retList.push_back(s);
		}
	}
	return retList;
}

auto Workspace::getAllRequiredPackages() const -> std::vector<PackageURL> {
	std::vector<PackageURL> retList;

	std::vector<Package> openPackages;
	{
		auto p = readPackage(path);
		openPackages.emplace_back(std::move(p));
	}
	while(not openPackages.empty()) {
		Package p = openPackages.back();
		openPackages.pop_back();
		for (auto const& p2 : p.getExtRepositories()) {
			if (estd::find(retList, p2) == retList.end()) {
				retList.push_back(p2);

				try {
					PackageURL url{p2};
					auto p = readPackage(url.getPath(), url);
					openPackages.emplace_back(std::move(p));
				} catch(...) {}
			}
		}
	}
	return retList;
}
auto Workspace::getAllNotRequiredPackages() const -> std::vector<std::string> {
	auto allRequired = getAllRequiredPackages();
	auto allPackages = utils::listDirs(path + "extRepositories", true);

	for (auto const& s : allRequired) {
		PackageURL url {s};
		auto iter = estd::find(allPackages, url.getName());
		if (iter != allPackages.end()) {
			allPackages.erase(iter);
		}
	}
	return allPackages;
}
auto Workspace::getAllRequiredProjects()    const -> std::map<std::string, Project> {
	std::map<std::string, Project> retList;

	// Adding root package projects
	{
		auto p = readPackage(path);

		for (auto project : p.getProjects()) {
			retList[project.getPath()] = project;
		}
	}

	// Adding all dependent projects
	auto required = getAllRequiredPackages();
	for (auto url : required) {
		auto package = readPackage(url.getPath(), url);
		for (auto project : package.getProjects()) {
			retList[project.getName()] = project;
		}
	}
	return retList;
}
auto Workspace::getExcludedProjects() const -> std::set<std::string> {
	std::set<std::string> retList;
	// Adding root package projects
	{
		auto p = readPackage(path);
		for (auto o : p.getOverrides()) {
			auto chains = o.getExcludeFromToolchains();
			if (std::find(chains.begin(), chains.end(), configFile.getToolchain()) != chains.end()) {
				retList.insert(o.getProjectToOverride());
			}
		}
	}

	// Adding all dependent projects
	auto required = getAllRequiredPackages();
	for (auto url : required) {
		auto package = readPackage(url.getPath(), url);
		for (auto o : package.getOverrides()) {
			auto chains = o.getExcludeFromToolchains();
			if (std::find(chains.begin(), chains.end(), configFile.getToolchain()) != chains.end()) {
				retList.insert(o.getProjectToOverride());
			}
		}
	}
	return retList;
}


auto Workspace::getRootPackageName() const -> std::string {
	auto p = readPackage(".");
	return p.getName();

}



void Workspace::createPackageFolder() {
	// check if repository folder exists
	if (not utils::fileExists(path + "extRepositories")) {
		utils::mkdir(path + "extRepositories");
	}
}
void Workspace::createBusyFolder() {
	// check if repository folder exists
	if (not utils::fileExists(path + ".busy")) {
		utils::mkdir(path + ".busy");
	}
}



}
