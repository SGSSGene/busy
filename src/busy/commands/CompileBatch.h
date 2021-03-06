#pragma once

#include "Workspace.h"

#include <mutex>
#include <process/Process.h>
#include <string>

namespace busy {
namespace commands {

class CompileBatch {
public:
	std::set<Project const*> needsRecompile;
	std::set<std::string> fileWasRecompiled;
	std::vector<std::string> mRPaths;
private:
	bool& errorDetected;
	std::mutex& printMutex;
	std::string buildPath;
	std::string outPath;
	std::string buildModeName;
	bool verbose;
	Toolchain const* toolchain;
	std::set<Project const*> ignoreProjects;
	bool mDryRun;
	bool mStrict;

	struct DBEntry {
		std::string file;
		std::string command;
	};
	std::vector<DBEntry> mDBEntries;
	bool                 mGenerateDBEntries;

public:
	CompileBatch(bool& _errorDetected, std::mutex& _printMutex, std::string _buildPath, std::string _outPath, std::string _buildModeName, bool _verbose, Toolchain const* _toolchain, std::set<Project const*> _ignoreProjects, bool _dryRun, bool _strict, bool _generateDBEntries)
		: errorDetected(_errorDetected)
		, printMutex(_printMutex)
		, buildPath (std::move(_buildPath))
		, outPath   (std::move(_outPath))
		, buildModeName (std::move(_buildModeName))
		, verbose(_verbose)
		, toolchain(_toolchain)
		, ignoreProjects(_ignoreProjects)
		, mDryRun(_dryRun)
		, mStrict(_strict)
		, mGenerateDBEntries(_generateDBEntries)
	{}

	void compileCpp(Project const* _project, std::string const& _file);
	void compileC(Project const* _project, std::string const& _file);
	void linkStaticLibrary(Project const* _project);
	void linkExecutable(Project const* _project);
	void linkSharedLibrary(Project const* _project);
	void linkPlugin(Project const* _project);

	auto getDBEntries() const -> std::vector<DBEntry> const& {
		return mDBEntries;
	}
private:
	void compile(Project const* _project, std::string const& _file, Toolchain::Command const& command);
	bool checkNeedsRecompile(std::string const& _file, std::string const& outputFile);
	void markProjectAndFileAsRecompiled(Project const* _project, std::string const& _file);
	auto generateDefines(Project const* _project) -> std::vector<std::string>;
	auto generateIncludes(Project const* _project) -> std::vector<std::string>;

	auto objectFilesForLinking(Project const* _project, std::string const& _buildPath) const -> std::vector<std::string>;

	bool runCmd(std::vector<std::string> const& _options);
	void printError(std::vector<std::string> const& _options, process::Process const& _proc);
};

}
}

