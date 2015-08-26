#include "Workspace.h"

#include "utils.h"
#include "estd.h"

namespace aBuild {

Workspace::Workspace(std::string const& _path)
	: path {_path + "/"} {

	if (utils::fileExists(path + ".aBuild/workspace.json")) {
		jsonSerializer::read(path + ".aBuild/workspace.json", configFile);
	}
	createPackageFolder();
}
Workspace::~Workspace() {
	jsonSerializer::write(path + ".aBuild/workspace.json", configFile);
}

auto Workspace::getAllMissingPackages() const -> std::vector<PackageURL> {
	std::set<PackageURL> urlSet;

	auto requiredPackages = getAllRequiredPackages();
	auto allPackages      = utils::listDirs(path + "packages", true);

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
	// Reading package directory finding available packages
	auto allPackages = utils::listDirs(path + "packages", true);
	for (auto const& s : allPackages) {
		try {
			std::string path2 = path + "packages/" + s;
			Package p {PackageURL()};
			jsonSerializer::read(path2 + "/aBuild.json", p);
			retList.push_back(std::move(p));
		} catch (...) {}
	}
	return retList;
}
auto Workspace::getAllInvalidPackages() const -> std::vector<std::string> {
	std::vector<std::string> retList;
	// Reading package directory finding available packages
	auto allPackages = utils::listDirs(path + "packages", true);
	for (auto const& s : allPackages) {
		try {
			std::string path2 = path + "packages/" + s;
			Package p {PackageURL()};
			jsonSerializer::read(path2 + "/aBuild.json", p);
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
		Package p {PackageURL()};
		jsonSerializer::read(path + "aBuild.json", p);
		openPackages.push_back(std::move(p));
	}
	while(not openPackages.empty()) {
		Package p = openPackages.back();
		openPackages.pop_back();
		for (auto const& p2 : p.getExtDependencies()) {
			if (estd::find(retList, p2) == retList.end()) {
				retList.push_back(p2);

				PackageURL url{p2};
				Package p {url};
				try {
					jsonSerializer::read(url.getPath() + "/aBuild.json", p);
					openPackages.push_back(std::move(p));
				} catch(...) {}
			}
		}
	}
	return retList;
}
auto Workspace::getAllNotRequiredPackages() const -> std::vector<std::string> {
	auto allRequired = getAllRequiredPackages();
	auto allPackages = utils::listDirs(path + "packages", true);

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
	//
	Package p {PackageURL()};
	jsonSerializer::read(path + "aBuild.json", p);
	for (auto project : p.getProjects()) {
		retList[project.getPath()] = project;
	}

	// Adding all dependent projects
	auto required = getAllRequiredPackages();
	for (auto url : required) {
		Package package {url};
		jsonSerializer::read(url.getPath() + "/aBuild.json", package);

		for (auto project : package.getProjects()) {
			retList[project.getName()] = project;
		}
	}
	return retList;
}



void Workspace::createPackageFolder() {
	// check if packages folder exists
	if (not utils::fileExists(path + "packages")) {
		utils::mkdir(path + "packages");
	}
}


}
