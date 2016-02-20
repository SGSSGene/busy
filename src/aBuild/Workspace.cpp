#include "Workspace.h"
#include <iostream>

#include "utils.h"
#include "estd.h"

#include <iostream>

namespace aBuild {

static void convertJsonToYaml(std::string const& _str) {
	// converting yaml files to json files
	if (utils::fileExists(_str + "/aBuild.json")
	    and not utils::fileExists(_str + "/aBuild.yaml")) {
		Package package {PackageURL()};
		serializer::json::read(_str + "/aBuild.json", package);
		serializer::yaml::write(_str + "/aBuild.yaml", package);
		std::cout << "converting " << _str << "/aBuild.json to " << _str + "/aBuild.yaml" << std::endl;
	}
}

Workspace::Workspace(std::string const& _path)
	: path {_path + "/"} {

	if (utils::fileExists(path + ".aBuild/workspace.yaml")) {
		serializer::yaml::read(path + ".aBuild/workspace.yaml", configFile);
	}
	createABuildFolder();
	createPackageFolder();
}
Workspace::~Workspace() {
	save();
}

void Workspace::save() {
	serializer::yaml::write(path + ".aBuild/workspace.yaml", configFile);
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
auto Workspace::getAllValidPackages(bool _includingRoot) const -> std::vector<Package> {
	std::vector<Package> retList;
	// Reading package directory finding available packages
	auto allPackages = utils::listDirs(path + "packages", true);
	for (auto const& s : allPackages) {
		try {
			std::string path2 = path + "packages/" + s;
			convertJsonToYaml(path2);
			Package p {PackageURL()};
			serializer::yaml::read(path2 + "/aBuild.yaml", p);
			retList.push_back(std::move(p));
		} catch (...) {}
	}
	if (_includingRoot) {
		Package p {PackageURL()};
		serializer::yaml::read("./aBuild.yaml", p);
		retList.push_back(std::move(p));
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
			convertJsonToYaml(path2);
			Package p {PackageURL()};
			serializer::yaml::read(path2 + "/aBuild.yaml", p);
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
		convertJsonToYaml(path);
		serializer::yaml::read(path + "aBuild.yaml", p);
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
					convertJsonToYaml(url.getPath());
					serializer::yaml::read(url.getPath() + "/aBuild.yaml", p);
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
	{
		Package p {PackageURL()};
		convertJsonToYaml(path);
		serializer::yaml::read(path + "aBuild.yaml", p);

		for (auto project : p.getProjects()) {
			retList[project.getPath()] = project;
		}
	}

	// Adding all dependent projects
	auto required = getAllRequiredPackages();
	for (auto url : required) {
		Package package {url};
		convertJsonToYaml(url.getPath());
		serializer::yaml::read(url.getPath() + "/aBuild.yaml", package);
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
		Package p {PackageURL()};
		convertJsonToYaml(path);
		serializer::yaml::read(path + "aBuild.yaml", p);
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
		Package package {url};
		convertJsonToYaml(url.getPath());
		serializer::yaml::read(url.getPath() + "/aBuild.yaml", package);
		for (auto o : package.getOverrides()) {
			auto chains = o.getExcludeFromToolchains();
			if (std::find(chains.begin(), chains.end(), configFile.getToolchain()) != chains.end()) {
				retList.insert(o.getProjectToOverride());
			}
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
void Workspace::createABuildFolder() {
	// check if packages folder exists
	if (not utils::fileExists(path + ".aBuild")) {
		utils::mkdir(path + ".aBuild");
	}
}



}
