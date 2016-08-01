#pragma once

#include "ProjectLegacy.h"


namespace busyConfig {
	using StringVector = std::vector<std::string>;
	struct Project {
		std::string   name;
		StringVector  dependencies;
		StringVector  optionalDependencies;
		std::string   type;
		ProjectLegacy legacy;
		StringVector  depLibraries;
		StringVector  defines;
		bool          noWarnings   { false };
		bool          wholeArchive { false };
		bool          mAutoDependenciesDiscovery { true };
		bool          mIgnore      { false };
		StringVector  linkAsShared;
		bool          mSingleFileProjects {false};

		template <typename Node>
		void serialize(Node& node) {
			node["name"]                 % name;
			node["dependencies"]         % dependencies;
			node["optionalDependencies"] % optionalDependencies;
			node["type"]                 % type                 or getDefaultTypeByName();
			node["legacy"]               % legacy;
			node["depLibraries"]         % depLibraries;
			node["defines"]              % defines;
			node["noWarnings"]           % noWarnings           or bool(false);
			node["wholeArchive"]         % wholeArchive         or bool(false);
			node["autoDependenciesDiscovery"] % mAutoDependenciesDiscovery or bool(true);
			node["ignore"]               % mIgnore              or bool(false);
			node["linkAsShared"]         % linkAsShared; // !TODO remove it is deprecated
			node["singleFileProjects"]   % mSingleFileProjects or bool(false);
		}
	private:
		auto getDefaultTypeByName() const -> std::string;
	};
}
