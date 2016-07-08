#include "PackageURL.h"

#include "utils.h"

namespace aBuild {

PackageURL::PackageURL()
	: url {"_"}
	, path {"."} {
}
auto PackageURL::getName() const -> std::string {
	auto name = url;
	if (utils::isEndingWith(name, ".git")) {
		for (int i {0}; i<4; ++i) name.pop_back();
	}
	auto l = utils::explode(name, "/");
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
