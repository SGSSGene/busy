#pragma once

#include <serializer/serializer.h>


namespace aBuild {

	class Override {
	private:
		std::string              projectToOverride;
		std::vector<std::string> excludeFromToolchains;
	public:
		template <typename Node>
		void serialize(Node& node) {
			node["projectToOverride"]     % projectToOverride;
			node["excludeFromToolchains"] % excludeFromToolchains;
		}

		auto getProjectToOverride() const -> std::string const&;
		auto getExcludeFromToolchains() const -> std::vector<std::string> const&;

	};

}
