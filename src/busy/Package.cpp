#include "Package.h"

namespace busy {

Package::Package(PackageURL const& _url)
	: url {_url} {}


auto Package::getName() const -> std::string const& {
	return name;
}
void Package::setName(std::string const& _name) {
	name = _name;
}

auto Package::getURL() const -> PackageURL const& {
	return url;
}

auto Package::getExtRepositories() const -> ExtRepositories const& {
	return extRepositories;
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



}
