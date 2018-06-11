#include "Package.h"

#include "Workspace.h"
#include <algorithm>
#include <busyConfig/busyConfig.h>
#include <busyUtils/busyUtils.h>
#include <process/Process.h>

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

auto retrieveGccVersion(std::vector<std::string> const& _command) -> std::string {
	auto commandIter = _command.end();
	// find first gcc
	for (auto iter = _command.begin(); iter != _command.end(); ++iter) {
		if (utils::fileExists(*iter)) {
			commandIter = iter;
			break;
		}
	}
	// if no gcc found, throw exception
	if (commandIter == _command.end()) {
		throw std::runtime_error("unknown compiler");
	}

	process::Process p({*commandIter, "--version"});
	if (p.getStatus() != 0) {
		throw std::runtime_error("unknown compiler version");
	}
	auto output = p.cout();
	auto line    = utils::explode(output, "\n").at(0);
	auto version = utils::explode(utils::explode(line, ")").at(1), " ").at(0);
	auto versionParts = utils::explode(version, ".");
	auto realVersion = versionParts.at(0) + "." + versionParts.at(1);
	return realVersion;
}

auto retrieveClangVersion(std::vector<std::string> const& _command) -> std::string {
	auto commandIter = _command.end();
	// find first clang
	for (auto iter = _command.begin(); iter != _command.end(); ++iter) {
		if (utils::fileExists(*iter)) {
			commandIter = iter;
			break;
		}
	}
	// if no clang found, throw exception
	if (commandIter == _command.end()) {
		throw std::runtime_error("unknown compiler");
	}

	process::Process p({*commandIter, "--version"});
	if (p.getStatus() != 0) {
		throw std::runtime_error("unknown compiler");
	}
	auto output = p.cout();
	auto line    = utils::explode(output, "\n").at(0);
	auto version = utils::explode(line, " ").at(2);
	auto versionParts = utils::explode(version, ".");
	auto realVersion = versionParts.at(0) + "." + versionParts.at(1);
	return realVersion;
}


	auto getProgs(busy::Toolchain toolchain) -> busy::Toolchain {
		auto f = [](std::vector<std::string>& list) {
			for (auto const& p : list) {
				if (utils::fileExists(p)) {
					list = {p};
					return;
				}
			}
			list = {};
		};
		f(toolchain.archivist.searchPaths);
		f(toolchain.cCompiler.searchPaths);
		f(toolchain.cppCompiler.searchPaths);
		f(toolchain.linkExecutable.searchPaths);
		return toolchain;
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
			mFlavors[name].name          = configPackage.name + "/" + name;
			mFlavors[name].buildMode     = shared.second.buildMode;
			mFlavors[name].toolchain     = shared.second.toolchain;
			mFlavors[name].mLinkAsShared = shared.second.linkAsShared;
			mFlavors[name].mRPaths       = shared.second.rpath;
		}

		// loading toolchains
		for (auto const& configToolchain : configPackage.toolchains) {
			// Check version
			try {
				auto _d = utils::explode(configToolchain.version, "-");
				if (_d.size() < 2) continue;
				auto type = _d.at(0);
				auto version = _d.at(1);

				if (type == "gcc") {
					auto realVersion = retrieveGccVersion(configToolchain.cCompiler.searchPaths);
					if (realVersion != version) continue;
				} else if (type == "clang") {
					auto realVersion = retrieveClangVersion(configToolchain.cCompiler.searchPaths);
					if (realVersion != version) continue;
				} else {
					std::cerr << "unknown toolchain type: " + type << std::endl;
				}
				busy::Toolchain toolchain;
				toolchain = configToolchain;
				mToolchains[configToolchain.name] = getProgs(toolchain);
			} catch (...) {}
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
/*		for (auto& flavor : mFlavors) {
			for (auto const& s : flavor.second.mLinkAsSharedAsStrings) {
				flavor.second.mLinkAsShared.push_back(&mWorkspace->getProject(s));
			}
		}*/
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
