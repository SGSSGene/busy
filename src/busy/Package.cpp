#include "Package.h"

#include <iostream>

#include "NeoWorkspace.h"

#include <serializer/serializer.h>

namespace busy {

Package::Package(PackageURL const& _url, busyConfig::Package const& _package)
	: url {_url}
{
	setName(_package.name);
	for (auto const& e : _package.extRepositories) {
		extRepositories.emplace_back(e);
	}
	for (auto const& e : _package.projects) {
		projects.emplace_back(e);
	}
	for (auto const& e : _package.overrides) {
		overrides.emplace_back(e);
	}
	for (auto const& e : _package.installations) {
		installations.emplace_back(e);
	}
	for (auto const& e : _package.toolchains) {
		toolchains.emplace_back(e);
	}
	for (auto const& e : _package.flavors) {
		flavors[e.first] = Flavor(e.second);
	}
	for (auto& p : projects) {
		p.setPackagePath(path);
		p.setFullName(name + "/" + p.getName());
	}
}


Package::Package(PackageURL const& _url)
	: url {_url} {}


void Package::setWorkspace(NeoWorkspace* _workspace) {
	mWorkspace = _workspace;
	for (auto& p : projects) {
		p.setWorkspace(mWorkspace);
	}

}


auto Package::getName() const -> std::string const& {
	return name;
}
void Package::setName(std::string const& _name) {
	name = _name;

	for (auto& p : projects) {
		p.setFullName(name + "/" + p.getName());
	}

}

void Package::setPath(std::string const& _path) {
	path = _path;

	for (auto& p : projects) {
		p.setPackagePath(path);
	}
}
auto Package::getPath() const -> std::string const& {
	return path;
}

auto Package::getURL() const -> PackageURL const& {
	return url;
}

auto Package::getExtRepositories() const -> ExtRepositories const& {
	return extRepositories;
}
auto Package::getAllProjects() const -> Projects {
	auto retVal = projects;

	// If src folder exists, check for project that have no config entry
	if (utils::fileExists(path + "/src")) {
		auto allProjects      = utils::listDirs(path + "/src", true);

		for (auto const& project : allProjects) {
			bool found = false;
			for (auto const& p : projects) {
				if (p.getName() == project) {
					found = true;
					break;
				}
			}
			if (not found) {
				Project p;
				p.set(project);
				p.setPackagePath(path);
				p.setFullName(name + "/" + p.getName());
				p.setHasConfigEntry(false);
				retVal.push_back(p);
			}
		}
	}
	return retVal;
}

auto Package::getProjects() const -> Projects const& {
	return projects;
}
auto Package::accessProjects() -> Projects& {
	return projects;
}

auto Package::getOverrides() const -> Overrides const& {
	return overrides;
}

auto Package::getInstallations() const -> Installations const& {
	return installations;
}
auto Package::getToolchains() const -> Toolchains const& {
	return toolchains;
}
auto Package::getFlavors() const -> Flavors const& {
	return flavors;
}

auto readPackage(std::string const& _path, PackageURL _url) -> Package {
	static std::map<std::string, busyConfig::Package> packages;

	auto path = _path;
	if (path.size() == 2 and path == "./") {
		path = ".";
	}


	if (packages.find(path) == packages.end()) {
		auto configPackage = busyConfig::readPackage(path);
		packages[path] = configPackage;
	}

	Package package{_url, packages.at(path)};
	package.setPath(path);

	if (not utils::validPackageName(package.getName())) {
		throw std::runtime_error(path + "/busy.yaml containts invalid package name: \"" + package.getName() + "\"");
	}

	return package;
}




}
