#include "Package.h"

#include "Workspace.h"
#include <algorithm>
#include <busyConfig/busyConfig.h>
#include <busyUtils/busyUtils.h>

namespace busy {
	Package::Package(std::string const& _path, Workspace* _workspace)
		: mWorkspace(_workspace)
	{
		// set path
		auto path = _path;
		if (path.size() == 2 and path == "./") {
			path = ".";
		}
		mPath = path;


		// read config file, set all other data
		auto configPackage = busyConfig::readPackage(path);
		mName = configPackage.name;

		// loading all projects from config file
		for (auto configProject : configPackage.projects) {
			mProjects.emplace_back(configProject, this);
		}
		// loading all projects from src folder that are not in the config file
		if (utils::fileExists(mPath + "/src")) {
			for (auto const& f : utils::listDirs(mPath + "/src", true)) {
				// Check if this path aready exists
				bool found = false;
				for (auto const& p : mProjects) {
					if (p.getName() == f) {
						found = true;
						break;
					}
				}
				if (not found) {
					mProjects.emplace_back(f, this);
				}
			}
		}

		// setting all externalRepURLS
		for (auto const& url : configPackage.extRepositories) {
			auto name = url.url;
			if (utils::isEndingWith(name, ".git")) {
				for (int i {0}; i<4; ++i) name.pop_back();
			}
			auto l = utils::explode(name, "/");
			if (l.size() == 1) {
				name = l[0];
			}
			name = l[l.size()-1];

			l = utils::explode(name, ":");
			if (l .size() == 1) {
			 name = l[0];
			}
			name = l[l.size()-1];

			mExternalRepURLs.push_back({name, url.url, url.branch});
		}

		// loading Flavors
		for (auto const& shared : configPackage.flavors) {
			auto name = shared.first;
			mFlavors[name].buildMode = shared.second.buildMode;
			mFlavors[name].toolchain = shared.second.toolchain;
			mFlavors[name].mLinkAsSharedAsStrings = shared.second.linkAsShared;
		}

		// loading toolchains
		for (auto const& toolchain : configPackage.toolchains) {
			mToolchains[toolchain.name] = toolchain;
		}

		// loading Overrides
		for (auto const& o : configPackage.overrides) {
			Override over;
			over.project = o.projectToOverride;
			for (auto const& t : o.excludeFromToolchains) {
				over.toolchains.insert(t);
			}
			mOverrides.push_back(over);
		}
	}
	void Package::setupPackageDependencies() {
		for (auto const& url : mExternalRepURLs) {
			auto package = &mWorkspace->getPackage(url.name);
			mExternalPackages.push_back(package);
		}
		for (auto& flavor : mFlavors) {
			for (auto const& s : flavor.second.mLinkAsSharedAsStrings) {
				flavor.second.mLinkAsShared.push_back(&mWorkspace->getProject(s));
			}
		}
}
	auto Package::getAllDependendPackages() -> std::vector<Package*> {
		std::vector<Package*> packages = {this};
		for (auto package : getExternalPackages()) {
			for (auto p : package->getAllDependendPackages()) {
				if (std::find(packages.begin(), packages.end(), p) == packages.end()) {
					packages.push_back(p);
				}
			}
		}
		return packages;
	}


	bool Package::hasProject(std::string const& _name) const {
		for (auto const& project : mProjects) {
			if (project.getName() == _name) {
				return true;
			}
		}
		return false;
	}

	auto Package::getProject(std::string const& _name) const -> Project const& {
		for (auto const& project : mProjects) {
			if (project.getName() == _name) {
				return project;
			}
		}
		throw std::runtime_error("Couldn't find project " + _name + " inside of the package " + getName());
	}

}
