#include "Workspace.h"

#include <algorithm>
#include <busyUtils/busyUtils.h>
#include <iostream>
#include <serializer/serializer.h>


namespace busy {

namespace {
	std::string extRepPath { "./extRepositories" };
	std::string busyPath   { "./.busy" };
	std::string workspaceFile { ".busy/workspace.bin" };

}

Workspace::Workspace() {
	// check if certain folders exists
	for (std::string s : {extRepPath , busyPath}) {
		if (not utils::fileExists(s)) {
			utils::mkdir(s);
		}
	}

	if (utils::fileExists(workspaceFile)) {
		serializer::binary::read(workspaceFile, mConfig);
	}


	loadPackageFolders();
	loadPackages();
	discoverSystemToolchains();
}
Workspace::~Workspace() {
	utils::AtomicWrite atomic(workspaceFile);
	serializer::binary::write(atomic.getTempName(), mConfig);
	atomic.close();

//	serializer::yaml::write(workspaceFile + ".yaml", mConfig);
}

auto Workspace::getPackageFolders() const -> std::vector<std::string> const& {
	return mPackageFolders;
}
auto Workspace::getPackages() const -> std::list<Package> const& {
	return mPackages;
}

bool Workspace::hasPackage(std::string const& _name) const {
	for (auto const& package : mPackages) {
		if (package.getName() == _name) {
			return true;
		}
	}
	return false;
}

auto Workspace::getPackage(std::string const& _name) const -> Package const& {
	return const_cast<Workspace*>(this)->getPackage(_name);
}
auto Workspace::getPackage(std::string const& _name) -> Package& {
	for (auto& package : mPackages) {
		if (package.getName() == _name) {
			return package;
		}
	}
	throw std::runtime_error("Couldn't find packages with name: " + _name);
}

auto Workspace::getProject(std::string const& _name) const -> Project const& {
	auto parts = utils::explode(_name, "/");
	if (parts.size() > 2) {
		throw std::runtime_error("given invalid full project name: " + _name + ". It should look like this Package/Project");
	} else if (parts.size() == 2) {
		auto const& package = getPackage(parts[0]);
		return package.getProject(parts[1]);
	}

	std::vector<Project const*> matchList;
	for (auto const& package : mPackages) {
		for (auto const& project : package.getProjects()) {
			if (project.getName () == _name) {
				matchList.push_back(&project);
			}
		}
	}
	if (matchList.empty()) {
		throw std::runtime_error("Coudn't find prjoect with name: " + _name);
	}
	if (matchList.size() > 1) {
		std::string list;
		for (auto const& p : matchList) {
			list += "  - " + p->getFullName() + "\n";
		}
		throw std::runtime_error("Found multiple projects with this name, specifiy the packages they are in:" + list);
	}
	return *matchList.at(0);
}
auto Workspace::getProjectAndDependencies(std::string const& _name) const -> std::vector<Project const*> {
	auto ignoreProjects = getExcludedProjects(getSelectedToolchain());

	std::vector<Project const*> retProjects;

	std::set<Project const*> flagged;
	std::vector<Project const*> queued = retProjects;

	if (_name == "") {
		for (auto const& package : getPackages()) {
			for (auto const& project : package.getProjects()) {
				if (project.getType() == "executable" and ignoreProjects.count(&project) == 0) {
					queued.push_back(&project);
					flagged.insert(&project);
				}
			}
		}
	} else {
		queued.push_back(&getProject(_name));
		flagged.insert(queued.front());
	}
	// Run through queue and get all dependencies
	while (not queued.empty()) {
		auto project = queued.back();
		retProjects.push_back(project);
		queued.pop_back();
		for (auto depP : project->getDependencies()) {
			if (flagged.count(depP) == 0 && ignoreProjects.count(depP) == 0) {
				flagged.insert(depP);
				queued.push_back(depP);
			}
		}
	}
	std::sort(retProjects.begin(), retProjects.end(), [](Project const* p1, Project const* p2) {
		return p1->getFullName() < p2->getFullName();
	});
	return retProjects;
}
auto Workspace::getFlavors() const -> std::map<std::string, Flavor const*> {
	std::map<std::string, Flavor const*> retMap;

	for (auto const& package : mPackages) {
		for (auto const& flavor : package.getFlavors()) {
			retMap[package.getName() + "/" + flavor.first] = &flavor.second;
		}
	}
	return retMap;
}
auto Workspace::getToolchains() const -> std::map<std::string, Toolchain const*> {
	std::map<std::string, Toolchain const*> retMap;
	for (auto const& toolchain : mSystemToolchains) {
		retMap[toolchain.first] = &toolchain.second;
	}
	for (auto const& package : mPackages) {
		for (auto const& toolchain : package.getToolchains()) {
			retMap[toolchain.first] = &toolchain.second;
		}
	}
	return retMap;
}
auto Workspace::getBuildModes() const -> std::vector<std::string> {
	return {"debug", "release"};
}


auto Workspace::getSelectedToolchain() const -> std::string {
	auto value = mConfig.mToolchainName;

	auto toolchains = getToolchains();
	auto iter = toolchains.find(value);
	if (iter == toolchains.end()) {
		return "system-gcc";
	}
	return iter->first;
}
auto Workspace::getSelectedBuildMode() const -> std::string {
	auto modes = getBuildModes();
	auto iter = std::find(modes.begin(), modes.end(), mConfig.mBuildModeName);
	if (iter == modes.end()) {
		return getBuildModes().front();
	}
	return *iter;
}

void Workspace::setSelectedToolchain(std::string const& _toolchainName) {
	auto toolchains = getToolchains();
	auto iter = toolchains.find(_toolchainName);
	if (iter == toolchains.end()) {
		throw std::runtime_error("Can not set toolchain to " + _toolchainName);
	}
	mConfig.mToolchainName = _toolchainName;

}
void Workspace::setSelectedBuildMode(std::string const& _buildMode) {
	auto modes = getBuildModes();
	auto iter = std::find(modes.begin(), modes.end(), _buildMode);
	if (iter == modes.end()) {
		throw std::runtime_error("Can not set build mode to " + _buildMode);
	}
	mConfig.mBuildModeName = _buildMode;
}

void Workspace::setFlavor(std::string const& _flavor) {
	// search for direct match
	for (auto flavor : getFlavors()) {
		if (flavor.first == _flavor) {
			setSelectedToolchain(flavor.second->toolchain);
			setSelectedBuildMode(flavor.second->buildMode);
			std::cout << "applying flavor " << _flavor << std::endl;
			return;
		}
	}

	// if no direct match was found, try indirect match (by leaving out the package name
	std::vector<Flavor const*> matches;
	for (auto flavor : getFlavors()) {
		auto parts = utils::explode(flavor.first, "/");
		if (parts[parts.size()-1] == _flavor) {
			matches.push_back(flavor.second);
		}
	}
	if (matches.size() == 0) {
		throw std::runtime_error("flavor " + _flavor + " is unknown");
	}
	if (matches.size() > 1) {
		throw std::runtime_error("flavor " + _flavor + " is ambigious");
	}
	setSelectedToolchain(matches.at(0)->toolchain);
	setSelectedBuildMode(matches.at(0)->buildMode);
}




auto Workspace::getFileStat(std::string const& _file) -> FileStat& {
	return mConfig.mFileStats[_file];
}
auto Workspace::getExcludedProjects(std::string const& _toolchain) const -> std::set<Project const*> {
	std::set<Project const*> projects;
	for (auto const& package : mPackages) {
		for (auto const& over : package.getOverrides()) {
			if (over.toolchains.count(_toolchain) > 0) {
				projects.insert(&getProject(over.project));
			}
		}
	}
	return projects;
}


void Workspace::loadPackageFolders() {
	mPackageFolders.emplace_back("./");
	for (auto const& f : utils::listDirs(extRepPath, true)) {
		mPackageFolders.emplace_back(extRepPath + "/" + f);
	}
}
void Workspace::loadPackages() {
	auto const& folders = getPackageFolders();

	for (auto const& f : folders) {
		mPackages.emplace_back(f, this);
	}
	for (auto& package : mPackages) {
		package.setupPackageDependencies();
	}
	for (auto& package : mPackages) {
		for (auto& project : package.getProjects()) {
			project.discoverDependencies();
		}
	}
}

void Workspace::discoverSystemToolchains() {
	std::map<std::string, std::pair<std::string, Toolchain>> searchPaths {
	     {"/usr/bin/gcc",       {"system-gcc",       {{"gcc"},       {"g++"},         {"ar"}}}},
	     {"/usr/bin/gcc-5.3",   {"system-gcc-5.3",   {{"gcc-5.3"},   {"g++-5.3"},     {"ar"}}}},
	     {"/usr/bin/gcc-5.2",   {"system-gcc-5.2",   {{"gcc-5.2"},   {"g++-5.2"},     {"ar"}}}},
	     {"/usr/bin/gcc-5.1",   {"system-gcc-5.1",   {{"gcc-5.1"},   {"g++-5.1"},     {"ar"}}}},
	     {"/usr/bin/gcc-5.0",   {"system-gcc-5.0",   {{"gcc-5.0"},   {"g++-5.0"},     {"ar"}}}},
	     {"/usr/bin/gcc-4.9",   {"system-gcc-4.9",   {{"gcc-4.9"},   {"g++-4.9"},     {"ar"}}}},
	     {"/usr/bin/gcc-4.8",   {"system-gcc-4.8",   {{"gcc-4.8"},   {"g++-4.8"},     {"ar"}}}},
	     {"/usr/bin/gcc-4.7",   {"system-gcc-4.7",   {{"gcc-4.7"},   {"g++-4.7"},     {"ar"}}}},
	     {"/usr/bin/clang",     {"system-clang",     {{"clang"},     {"clang++"},     {"ar"}}}},
	     {"/usr/bin/clang-3.6", {"system-clang-3.6", {{"clang-3.6"}, {"clang++-3.6"}, {"ar"}}}},
	     {"/usr/bin/clang-3.8", {"system-clang-3.8", {{"clang-3.8"}, {"clang++-3.8"}, {"ar"}}}},
	     };

	for (auto const& p : searchPaths) {
		if (utils::fileExists(p.first)) {
			mSystemToolchains[p.second.first] = p.second.second;
		}
	}
}






}
