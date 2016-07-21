#include "PackageURL.h"

#include <busyUtils/busyUtils.h>

namespace busy {

PackageURL::PackageURL()
	: url {"_"}
	, path {"."} {
}
PackageURL::PackageURL(busyConfig::PackageURL const& _url) {
	url    = _url.url;
	branch = _url.branch;

	path = "extRepositories/"+getName();
}

auto PackageURL::getName() const -> std::string {
	auto name = url;
	if (utils::isEndingWith(name, ".git")) {
		for (int i {0}; i<4; ++i) name.pop_back();
	}
	auto l = utils::explode(name, "/");
	if (l.size() == 1) {
		return l[0];
	}

	name = l[l.size()-1];
	return name;
}
auto PackageURL::getURL() const -> std::string const& {
	return url;
}
auto PackageURL::getBranch() const -> std::string const& {
	return branch;
}
auto PackageURL::getPath() const -> std::string const& {
	return path;
}
bool PackageURL::operator<(PackageURL const& _other) const {
	return getName() < _other.getName();
}
bool PackageURL::operator==(PackageURL const& _other) const {
	return getName() == _other.getName();
}

}
