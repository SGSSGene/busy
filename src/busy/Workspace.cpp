#include "Workspace.h"

#include <algorithm>
#include <busyUtils/busyUtils.h>
#include <iostream>
#include <serializer/serializer.h>
#include <process/Process.h>


namespace busy {

namespace {
	std::string extRepPath { "./extRepositories" };
	std::string busyPath   { "./.busy" };
	std::string workspaceFile { ".busy/workspace.bin" };

}

Workspace::Workspace(bool _noSaving)
	: mNoSaving { _noSaving }
{
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
	if (not mNoSaving) {
		utils::AtomicWrite atomic(workspaceFile);
		serializer::binary::write(atomic.getTempName(), mConfig);
		atomic.close();

//		serializer::yaml::write(workspaceFile + ".yaml", mConfig);
	}
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
bool Workspace::hasProject(std::string const& _name) const {
	auto parts = utils::explode(_name, "/");
	if (parts.size() > 2) {
		throw std::runtime_error("given invalid full project name: " + _name + ". It should look like this Package/Project");
	} else if (parts.size() == 2) {
		if (not hasPackage(parts[0])) {
			return false;
		}
		auto const& package = getPackage(parts[0]);
		return package.hasProject(parts[1]);
	}

	for (auto const& package : mPackages) {
		for (auto const& project : package.getProjects()) {
			if (project.getName () == _name) {
				return true;
			}
		}
	}
	return false;
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
auto Workspace::getProject(std::string const& _name) -> Project& {
	Project const& p = ((Workspace const*)(this))->getProject(_name);
	return const_cast<Project&>(p);
}

auto Workspace::getProjectAndDependencies(std::string const& _name) const -> std::vector<Project const*> {
	auto ignoreProjects = getExcludedProjects(getSelectedToolchain());

	std::vector<Project const*> retProjects;

	std::set<Project const*> flagged;
	std::vector<Project const*> queued = retProjects;

	if (_name == "") {
		for (auto const& package : getPackages()) {
			for (auto const& project : package.getProjects()) {
				if (project.getType() == Project::Type::Executable and ignoreProjects.count(&project) == 0) {
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
	return {"debug", "release", "release_with_symbols"};
}


auto Workspace::getSelectedToolchain() const -> std::string {
	auto value = mConfig.mToolchainName;

	auto toolchains = getToolchains();
	auto iter = toolchains.find(value);
	if (iter == toolchains.end()) {
		return "default-gcc";
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
	Flavor const* flavorPtr = nullptr;
	for (auto const& flavor : getFlavors()) {
		if (flavor.first == _flavor) {
			flavorPtr = flavor.second;
			break;
		}
	}

	// if no direct match was found, try indirect match (by leaving out the package name
	if (flavorPtr == nullptr) {
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
		flavorPtr = matches.at(0);
	}
	setSelectedToolchain(flavorPtr->toolchain);
	setSelectedBuildMode(flavorPtr->buildMode);
	mConfig.mStaticAsShared.clear();
	for (auto const& s : flavorPtr->mLinkAsShared) {
		mConfig.mStaticAsShared.push_back(s);
	}
	mConfig.mRPaths = flavorPtr->mRPaths;

	std::cout << "Applying flavor " << flavorPtr->name << "\n";
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
void Workspace::markProjectAsShared(std::string const& _name) {
	for (auto p : getSharedProjects()) {
		if (p->getType() != Project::Type::StaticLibrary) continue;
		p->setType(Project::Type::SharedLibrary);
	}
}
auto Workspace::getRPaths() const -> std::vector<std::string> const& {
	return mConfig.mRPaths;
}


auto Workspace::getSharedProjects() -> std::set<Project*> {
	std::set<Project*> projects;
	for (auto const& s : mConfig.mStaticAsShared) {
		projects.insert(&getProject(s));
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

namespace {
auto retrieveGccVersion(std::vector<std::string> const& _command) -> std::string {
	auto commandIter = _command.end();
	// find first gcc
	for (auto iter = _command.begin(); iter != _command.end(); ++iter) {
		if (iter->find("gcc") != std::string::npos
		    or iter->find("g++" ) != std::string::npos) {
			commandIter = iter;
			break;
		}
	}
	// if no gcc found, throw exception
	if (commandIter == _command.end()) {
		throw std::runtime_error("unknown compiler");
	}

	process::Process p({*commandIter, "--version"});
	if (p.getStatus() != 0) return "";
	auto output = p.cout();
	auto line    = utils::explode(output, "\n").at(0);
	auto version = utils::explode(utils::explode(line, ")").at(1), " ").at(0);
	auto versionParts = utils::explode(version, ".");
	auto realVersion = versionParts.at(0) + "." + versionParts.at(1);
	return realVersion;
}

auto retrieveClangVersion(std::vector<std::string> const& _command) -> std::string {
	auto commandIter = _command.end();
	// find first gcc
	for (auto iter = _command.begin(); iter != _command.end(); ++iter) {
		if (iter->find("clang") != std::string::npos
		    or iter->find("clang++" ) != std::string::npos) {
			commandIter = iter;
			break;
		}
	}
	// if no gcc found, throw exception
	if (commandIter == _command.end()) {
		throw std::runtime_error("unknown compiler");
	}

	process::Process p({*commandIter, "--version"});
	if (p.getStatus() != 0) return "";
	auto output = p.cout();
	auto line    = utils::explode(output, "\n").at(0);
	auto version = utils::explode(line, " ").at(2);
	auto versionParts = utils::explode(version, ".");
	auto realVersion = versionParts.at(0) + "." + versionParts.at(1);
	return realVersion;
}

}

void Workspace::discoverSystemToolchains() {
	// setup toolchains plus fallback toolchain
	std::map<std::string, std::pair<std::string, Toolchain>> searchPaths {
	     {"/usr/bin/gcc",       {"default-gcc", {"", false,
	                                             {{"gcc", "-std=c11",   "-Wall", "-Wextra", "-fmessage-length=0", "-fPIC", "-rdynamic", "-MD"}, {}},
	                                             {{"g++", "-std=c++11", "-Wall", "-Wextra", "-fmessage-length=0", "-fPIC", "-rdynamic", "-MD"}, {}},
	                                             {{"ar"}, {}}
	                                            }
	                            }
	     }
	};
	for (auto const& p : searchPaths) {
		mSystemToolchains[p.second.first] = p.second.second;
	}

	// check /usr/share/busy/toolchains
	std::string resources = "/usr/share/busy/toolchains";

	if (utils::fileExists(resources)) {
		utils::walkFiles(resources, [&](std::string _file) {
			if (not utils::isEndingWith(_file, ".yaml")) return;
			if (utils::isStartingWith(_file, ".")) return;

			busyConfig::Package package;
			serializer::yaml::read(resources + "/" + _file , package);
			for (auto configToolchain : package.toolchains) {
				// Check version
				try {
					auto version     = configToolchain.version;
					if (configToolchain.type == "gcc") {
						auto realVersion = retrieveGccVersion(configToolchain.cCompiler.command);
						if (realVersion != version) continue;
					} else if (configToolchain.type == "clang") {
						auto realVersion = retrieveClangVersion(configToolchain.cCompiler.command);
						if (realVersion != version) continue;
					} else {
						std::cerr << "unknown toolchain type: " + configToolchain.type << std::endl;
					}
					mSystemToolchains[configToolchain.name] = configToolchain;
				} catch (...) {}

			}
		});
	}
}






}
