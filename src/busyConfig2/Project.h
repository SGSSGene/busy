#pragma once

#include "ProjectLegacy.h"


namespace busyConfig {

	using StringVector = std::vector<std::string>;
	struct Project {
		std::string   name;
		StringVector  dependencies;
		StringVector  optionalDependencies;
		std::string   type {"library"};
		ProjectLegacy legacy;
		StringVector  depLibraries;
		StringVector  defines;
		bool          noWarnings   { false };
		bool          warningsAsErrors { true };
		bool          wholeArchive { false };
		bool          mAutoDependenciesDiscovery { true };
		bool          mIgnore      { false };
		bool          mSingleFileProjects {false};
		bool          mPlugin {false};

		template <typename Node>
		void serialize(Node& node) {
			node["name"]                 % name;
			node["dependencies"]         % dependencies;
			node["optionalDependencies"] % optionalDependencies;
			node["type"]                 % type;
			node["legacy"]               % legacy;
			node["depLibraries"]         % depLibraries;
			node["defines"]              % defines;
			node["noWarnings"]           % noWarnings;
			node["warningsAsErrors"]     % warningsAsErrors;
			node["wholeArchive"]         % wholeArchive;
			node["autoDependenciesDiscovery"] % mAutoDependenciesDiscovery;
			node["ignore"]               % mIgnore;
			node["singleFileProjects"]   % mSingleFileProjects;
		}
	private:
		auto getDefaultTypeByName() const -> std::string;
	};
}
