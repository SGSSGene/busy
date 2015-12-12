#include "Project.h"

#include "Workspace.h"
#include <algorithm>
#include <iostream>
#include <fstream>
namespace aBuild {

void Project::quickFix() {
//!TODO sometimes wrong dependencies get detected, this is bad
/*	for (auto const& d : getDefaultDependencies()) {
		if (std::find(dependencies.begin(), dependencies.end(), d) == dependencies.end()) {
			dependencies.push_back(d);
		}
	}*/
	std::sort(dependencies.begin(), dependencies.end());
}


/** tries to determine type by name
 *  name starting with "test" are executables
 *  everything else is a library
 */
auto Project::getDefaultTypeByName() const -> std::string {
	if (utils::isStartingWith(path, "test")) {
		return "executable";
	}
	return "library";
}

auto Project::getDefaultDependencies() const -> Dependencies {
	Dependencies dep;
	auto allFiles = getAllFiles({".cpp", ".c", ".h"});
	std::set<std::pair<std::string, std::string>> filesOfInterest;
	for (auto const& f : allFiles) {
		std::ifstream ifs(f);
		std::string line;
		while (std::getline(ifs, line)) {
			if (utils::isStartingWith(line, "#include <")
			    and utils::isEndingWith(line, ">")) {
				auto pos1 = line.find("<")+1;
				auto pos2 = line.find(">")-pos1;
				auto l = utils::explode(line.substr(pos1, pos2), "/");
				if (l.size() == 1) {
					filesOfInterest.insert({l[0], ""});
				} else if (l.size() == 2) {
					filesOfInterest.insert({l[0], l[1]});
				}
			}
		}
	}
	if (not utils::dirExists("./packages")) return dep;

	std::set<std::pair<std::string, std::string>> pathsOfInterest;

	for (auto packageName : utils::listDirs("packages", true)) {
		utils::Cwd cwd("packages/" + packageName);
		Package package {PackageURL{}};
		serializer::yaml::read("aBuild.yaml", package);
		for (auto const& project : package.getProjects()) {
			pathsOfInterest.insert( {package.getName(), project.getName()} );
		}
	}
	for (auto projectNames : utils::listDirs("src", true)) {
		pathsOfInterest.insert( {"../", projectNames} );
	}

	// get current package name
	auto dir = utils::explode(utils::cwd(), "/");
	std::string packageName = dir[dir.size()-1];

	for (auto const& f : filesOfInterest) {
		for (auto const& p : pathsOfInterest) {
			if ((f.second == "" and utils::fileExists("./packages/"+p.first+"/src/"+f.first))
			 or (f.second != "" and utils::fileExists("./packages/"+p.first+"/src/"+p.second+"/"+f.second))) {
				std::string x = p.first + "/" + p.second;
				if (p.first == "../") {
					x = packageName + "/" + p.second;
				}
				if (x == packageName + "/" + getName()) continue; // Same Project (self refering)
				bool found = false;
				for (auto const& f : dep) {
					if (f == x) {
						found = true;
						break;
					}
				}
				if (not found) {
					dep.push_back(x);
				}
			}
		}
	}
	dep.erase(std::remove_if(dep.begin(), dep.end(), [&](std::string const& s) {
		if (std::find(optionalDependencies.begin(), optionalDependencies.end(), s) != optionalDependencies.end()) {
			return true;
		}
		return false;
	}), dep.end());

	return dep;
}

auto Project::getAllFiles(std::set<std::string> const& _ending) const -> std::vector<std::string> {
	std::vector<std::string> files;

	std::string fullPath = getPackagePath()+"/src/"+getPath()+"/";

	if (not utils::dirExists("./"+fullPath)) return files;


	auto allFiles = utils::listFiles(fullPath, true);
	for (auto const& f : allFiles) {
		for (auto const& ending : _ending) {
			if (utils::isEndingWith(f, ending)) {
				files.push_back(fullPath + f);
				break;
			}
		}
	}
	return files;
}


auto Project::getAllCppFiles() -> std::vector<std::string>& {
	if (cppFiles.empty()) {
		cppFiles = getAllFiles({".cpp"});
	}
	return cppFiles;
}
auto Project::getAllCFiles() -> std::vector<std::string>& {
	if (cFiles.empty()) {
		cFiles = getAllFiles({".c"});
	}

	return cFiles;
}


}

