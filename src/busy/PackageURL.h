#pragma once

#include <serializer/serializer.h>

namespace aBuild {

	class PackageURL {
	private:
		std::string url;
		std::string branch;
		std::string path;
	public:
		PackageURL();
		auto getName() const -> std::string;
		auto getURL() const -> std::string const&;
		auto getBranch() const -> std::string const&;
		auto getPath() const -> std::string const&;
		bool operator<(PackageURL const& _other) const;
		bool operator==(PackageURL const& _other) const;

		template<typename Node>
		void serialize(Node& node) {
			node["url"]    % url;
			node["branch"] % branch or std::string("master");

			path = "extRepositories/"+getName();
		}
	};

}
