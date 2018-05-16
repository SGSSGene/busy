#include "Package.h"

#include "Workspace.h"
#include <algorithm>
#include <busyConfig/busyConfig.h>
#include <busyUtils/busyUtils.h>

#include <iostream>

namespace busy {
namespace {
	std::string shadowSrc {".busy/shadow-src"};

	auto sanitize(std::string const& _s) -> std::string {
		std::string r;
		for (char c : _s) {
			if ((c >= 'a' && c <= 'z')
			    or (c >= 'A' && c <= 'Z')
			    or (c >= '0' && c <= '9')
			    or (c == '-')) {
				r += c;
			} else {
				r += "_";
			}
		}
		return r;
	}

}

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

		if (utils::fileExists(shadowSrc + "/" + mName)) {
			utils::rm(shadowSrc + "/" + mName, true);
		}


		// loading all projects from config file
		for (auto configProject : configPackage.projects) {
			mProjects.emplace_back(configProject, this, getPath() + "/src");
			auto& proj = mProjects.back();
			if (proj.getIsSingleFileProjects()) {
				createShadowSource(proj);
			}
		}

		// loading all projects from src folder that are not in the config file
		for (auto path : std::vector<std::string> {mPath + "/src", "./" + shadowSrc + "/" + mName}) {
//			auto path = mPath + "/" + s;
			if (not utils::fileExists(path)) {
				continue;
			}

			for (auto const& f : utils::listDirs(path, true)) {
				// Check if this path aready exists
				bool found = false;
				for (auto const& p : mProjects) {
					if (p.getName() == f) {
						found = true;
						break;
					}
				}
				if (not found) {
					mProjects.emplace_back(f, this, path);
				}
			}
		}

		// loading Flavors
		for (auto const& shared : configPackage.flavors) {
			auto name = shared.first;
			mFlavors[name].name          = configPackage.name + "/" + name;
			mFlavors[name].buildMode     = shared.second.buildMode;
			mFlavors[name].toolchain     = shared.second.toolchain;
			mFlavors[name].mLinkAsShared = shared.second.linkAsShared;
			mFlavors[name].mRPaths       = shared.second.rpath;
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
		auto configPackage = busyConfig::readPackage(mPath);

		// adding external repos
		for (auto const& name : configPackage.extRepositories) {
			auto package = &mWorkspace->getPackage(name);
			mExternalPackages.push_back(package);
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
	void Package::createShadowSource(Project const& p) {

		if (not utils::fileExists(shadowSrc + "/" + mName)) {
			utils::mkdir(shadowSrc + "/" + mName);
		}

		auto replaceEnding = [](std::string const& s, int popCt, std::string const& ext) {
			return s.substr(0, s.size() - popCt) + ext;
		};


		for (auto file : p.getCppAndCFiles()) {
			auto projectName = sanitize(file);
			auto baseName    = file.substr(file.find_last_of("/"));

			auto projectPath = shadowSrc + "/" + mName + "/" + projectName;
			utils::mkdir(projectPath);
			auto linkFile = projectPath + "/" + baseName;
			auto targetFile = "../../../../" + file;
			auto headerFile = replaceEnding(file, 4, ".h");
			if (utils::fileExists(headerFile)) {
				utils::softlink(replaceEnding(targetFile, 4, ".h"), replaceEnding(linkFile, 4, ".h"));
			}

			utils::softlink(targetFile, linkFile);
			mProjects.emplace_back(projectName, this, shadowSrc + "/" + mName);
			auto& proj = mProjects.back();
			proj.setType(p.getType());
		}
	}

}
