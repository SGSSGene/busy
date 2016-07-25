#include "NeoWorkspace.h"
#include <busyUtils/busyUtils.h>
#include <iostream>
#include <serializer/serializer.h>


namespace busy {

namespace {
	std::string extRepPath { "./extRepositories" };
	std::string busyPath   { "./.busy" };
	std::string workspaceFile { ".busy/neoworkspace.bin" };

}

NeoWorkspace::NeoWorkspace() {
	// check if certain folders exists
	for (std::string s : {extRepPath , busyPath}) {
		if (not utils::fileExists(s)) {
			utils::mkdir(s);
		}
	}

	if (utils::fileExists(workspaceFile)) {
		serializer::binary::read(workspaceFile, mNeoFileStats);
	}


	loadPackageFolders();
	loadPackages();
}
NeoWorkspace::~NeoWorkspace() {
	utils::AtomicWrite atomic(workspaceFile);
	serializer::binary::write(atomic.getTempName(), mNeoFileStats);
	atomic.close();
}

auto NeoWorkspace::getPackageFolders() const -> std::vector<std::string> const& {
	return mPackageFolders;
}
auto NeoWorkspace::getPackages() const -> std::list<NeoPackage> const& {
	return mPackages;
}
bool NeoWorkspace::hasPackage(std::string const& _name) const {
	for (auto const& package : mPackages) {
		if (package.getName() == _name) {
			return true;
		}
	}
	return false;
}

auto NeoWorkspace::getPackage(std::string const& _name) const -> NeoPackage const& {
	return const_cast<NeoWorkspace*>(this)->getPackage(_name);
}
auto NeoWorkspace::getPackage(std::string const& _name) -> NeoPackage& {
	for (auto& package : mPackages) {
		if (package.getName() == _name) {
			return package;
		}
	}
	throw std::runtime_error("Couldn't find packages with name: " + _name);
}

auto NeoWorkspace::getProject(std::string const& _name) const -> NeoProject const& {
	auto parts = utils::explode(_name, "/");
	if (parts.size() > 2) {
		throw std::runtime_error("given invalid full project name: " + _name + ". It should look like this Package/Project");
	} else if (parts.size() == 2) {
		auto const& package = getPackage(parts[0]);
		return package.getProject(parts[1]);
	}

	std::vector<NeoProject const*> matchList;
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

auto NeoWorkspace::getFileStat(std::string const& _file) -> NeoFileStat& {
	return mNeoFileStats[_file];
}

void NeoWorkspace::loadPackageFolders() {
	mPackageFolders.emplace_back("./");
	for (auto const& f : utils::listDirs(extRepPath, true)) {
		mPackageFolders.emplace_back(extRepPath + "/" + f);
	}
}
void NeoWorkspace::loadPackages() {
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





}
