#pragma once

#include <busyConfig/busyConfig.h>

namespace busy {
	class NeoPackage;

	class NeoProject {
	private:
		NeoPackage* mPackage;

		std::string mName;
		std::string mPath;
		std::string mType {"library"};

		bool        mHasConfigEntry {false};
		bool        mWholeArchive {false};
		bool        mAutoDependenciesDiscovery {true};

		std::vector<std::string> mSourcePaths;
		std::vector<std::string> mIncludePaths;
		std::vector<std::string> mSystemIncludePaths;
		std::set<std::string>    mDependenciesAsString;
		std::vector<NeoProject const*> mDependencies;
		std::map<std::string, std::vector<std::string>> mSourceFiles;
	public:
		// Constructed from config file
		NeoProject(busyConfig::Project const& _project, NeoPackage* _package);

		// Constructed from folder
		NeoProject(std::string const& _name, NeoPackage* _package);

		auto getName() const -> std::string const& { return mName; }
		auto getFullName() const -> std::string;
		auto getPath() const -> std::string const& { return mPath; }
		bool getHasConfigEntry() const { return mHasConfigEntry; }
		auto getType() const -> std::string const& { return mType; }
		bool getWholeArchive() const { return mWholeArchive; }
		bool getAutoDependenciesDiscovery() const { return mAutoDependenciesDiscovery; }
		auto getSourcePaths() const -> std::vector<std::string> const& { return mSourcePaths; }
		auto getIncludePaths() const -> std::vector<std::string> const& { return mIncludePaths; }
		auto getSystemIncludePaths() const -> std::vector<std::string> const& { return mSystemIncludePaths; }
		auto getDependencies() const -> std::vector<NeoProject const*> const& { return mDependencies; }
		auto getCFiles() const -> std::vector<std::string> const& { return mSourceFiles.at("c"); }
		auto getCppFiles() const -> std::vector<std::string> const& { return mSourceFiles.at("cpp"); }
		auto getIncludeFiles() const -> std::vector<std::string> const& { return mSourceFiles.at("incl"); }
		auto getIncludeFilesFlat() const -> std::vector<std::string> const& { return mSourceFiles.at("incl-flat"); }

		auto getIncludeAndDependendPaths() const -> std::vector<std::string>;
		auto getSystemIncludeAndDependendPaths() const -> std::vector<std::string>;

	private:
		void discoverSourceFiles();
	public:
		void discoverDependencies();
	private:
		void discoverDependenciesInFile(std::string const& _file);

	};
}
