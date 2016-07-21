#pragma once

#include <busyConfig/busyConfig.h>

namespace busy {

	class PackageURL {
	private:
		std::string url;
		std::string branch;
		std::string path;
	public:
		PackageURL(busyConfig::PackageURL const& _url);
		PackageURL();
		auto getName() const -> std::string;
		auto getURL() const -> std::string const&;
		auto getBranch() const -> std::string const&;
		auto getPath() const -> std::string const&;
		bool operator<(PackageURL const& _other) const;
		bool operator==(PackageURL const& _other) const;
	};
}
