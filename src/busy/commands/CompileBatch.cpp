#include "CompileBatch.h"

#include <busyUtils/busyUtils.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <set>
#include <string>

namespace {
	auto getDependenciesFromFile(std::string const& _file) -> std::vector<std::string> {
		std::vector<std::string> depFiles;

		std::ifstream ifs(_file);
		for (std::string line; std::getline(ifs, line);) {
			depFiles.emplace_back(std::move(line));
		}
		return depFiles;
	}

	template <typename T>
	auto removeLastDuplicates(std::vector<T> const& vec) -> std::vector<T> {
		std::vector<T> retList;
		for (auto iter1 = vec.begin(); iter1 != vec.end(); ++iter1) {
			bool found = false;
			for (auto iter2 = iter1 +1; iter2 != vec.end(); ++iter2) {
				if (*iter1 == *iter2) {
					found = true;
					break;
				}
			}
			if (not found) {
				retList.push_back(*iter1);
			}
		}
		return retList;
	}




	auto substitute(std::vector<std::string> const& _options, std::map<std::string, std::vector<std::string>> const& _map) -> std::vector<std::string> {
		std::vector<std::string> options;
		for (auto const& o : _options) {
			auto iter = _map.find(o);
			if (iter != _map.end()) {
				auto prefix = o.substr(0, o.find('%'));
				for (auto const& o2 : iter->second) {
					for (auto s : utils::explode(prefix+o2, " ")) {
						options.push_back(s);
					}
				}
			} else if (o.find("%") != std::string::npos) {
				std::cerr << "\nignoring: " << o << std::endl;
			} else {
				options.push_back(o);
			}
		}
		return options;
	}
}


namespace busy {
namespace commands {

void CompileBatch::compileCpp(Project const* _project, std::string const& _file) {
	compile(_project, _file, toolchain->cppCompiler);
}

void CompileBatch::compileC(Project const* _project, std::string const& _file) {
	compile(_project, _file, toolchain->cCompiler);
}
void CompileBatch::linkStaticLibrary(Project const* _project) {
	if (mGenerateDBEntries) {
		return;
	}

	std::string outputFile = buildPath + "/" + _project->getFullName() + ".a";

	// Check file dependencies
	{
		std::lock_guard<std::mutex> lock(printMutex);
		if (needsRecompile.count(_project) == 0
		    && utils::fileExists(outputFile)) {
			return;
		}

		if (_project->getIsHeaderOnly()) return;
		needsRecompile.insert(_project);
	}

	auto const& _command = toolchain->archivist;
	std::map<std::string, std::vector<std::string>> subMap;
	subMap["%compiler%"]       = {_command.searchPaths.back()};
	subMap["%objfiles%"]       = objectFilesForLinking(_project, buildPath);
	subMap["%outfile%"]        = {outputFile};
	auto options = substitute(_command.call, subMap);

	runCmd(options);
}

void CompileBatch::linkSharedLibrary(Project const* _project) {
	std::string outputFile = outPath + "/lib" + _project->getName() + ".so";
	throw std::runtime_error("not implemented");
}

void CompileBatch::linkPlugin(Project const* _project) {
	std::string outputFile = outPath + "/plugin" + _project->getName() + ".so";
	throw std::runtime_error("not implemented");
}

void CompileBatch::linkExecutable(Project const* _project) {
	if (mGenerateDBEntries) {
		return;
	}
	std::string outputFile = outPath + "/" + _project->getName();
	if (_project->getIsUnitTest()) {
		outputFile = outPath + "/tests/" + _project->getFullName();
	} else if (_project->getIsExample()) {
		outputFile = outPath + "/examples/" + _project->getFullName();
	}

	// Check file dependencies
	{
		bool recompile = false;
		std::lock_guard<std::mutex> lock(printMutex);
		if (needsRecompile.count(_project) > 0) {
			recompile = true;
		} else if (not utils::fileExists(outputFile)) {
			recompile = true;
		}
		for (auto _p : _project->getDependenciesRecursiveOnlyStaticNotOverShared(ignoreProjects)) {
			if (needsRecompile.count(_p) > 0) {
				recompile = true;
				break;
			}
		}
		if (not recompile) {
			return;
		}
	}

	// add static libraries
	std::vector<std::string> staticFiles;
	for (auto project : _project->getDependenciesRecursiveOnlyStaticNotOverShared(ignoreProjects)) {
		if (project->getIsHeaderOnly()) continue;

		if (project->getWholeArchive()) {
			staticFiles.push_back("-Wl,--whole-archive");
		}
		staticFiles.push_back(buildPath + "/" + project->getFullName() + ".a");
		if (project->getWholeArchive()) {
			staticFiles.push_back("-Wl,--no-whole-archive");
		}
	}

	// generate library and library paths
	std::vector<std::string> systemLibraries;
	std::vector<std::string> systemLibrariesPaths;
	// add shared libraries
	for (auto project : _project->getDependenciesRecursiveOnlyShared(ignoreProjects)) {
		if (project->getIsHeaderOnly()) continue;

		std::string fullPath = outPath + "/";

		systemLibrariesPaths.push_back(fullPath);
		systemLibraries.push_back(project->getName());
	}

	for (auto dep : _project->getSystemLibraries()) {
		systemLibraries.push_back(dep);
	}
	for (auto project : _project->getDependenciesRecursive()) {
		for (auto dep : project->getSystemLibraries()) {
			systemLibraries.push_back(dep);
		}
	}

	for (auto project : _project->getDependenciesRecursive()) {
		for (auto linking : project->getSystemLibrariesPaths()) {
			systemLibrariesPaths.push_back(linking);
		}
	}

	systemLibrariesPaths = removeLastDuplicates(systemLibrariesPaths);
	systemLibraries      = removeLastDuplicates(systemLibraries);

	auto const& _command = toolchain->linkExecutable;
	std::map<std::string, std::vector<std::string>> subMap;
	subMap["%compiler%"]          = {_command.searchPaths.back()};
	subMap["%outfile%"]           = {outputFile};
	subMap["%objfiles%"]          = objectFilesForLinking(_project, buildPath);
	subMap["%afiles%"]            = staticFiles;
	subMap["-fuse-ld=%ld%"]       = {utils::getEnv("BUSY_LD", "")};
	subMap["-Wl,-rpath %rpaths%"] = mRPaths;
	subMap["%legacyLinking%"]     = _project->getLinkingOptionsRecursive();
	subMap["-L%libPaths%"]        = systemLibrariesPaths;
	subMap["-l%libs%"]            = systemLibraries;

	auto options = substitute(_command.call, subMap);

	runCmd(options);
}

void CompileBatch::compile(Project const* _project, std::string const& _file, Toolchain::Command const& _command) {
	if (errorDetected) return;
	if (_project->getIsSingleFileProjects()) return;

	std::string outputFile = buildPath + "/" + _file + ".o";

	// Check file dependencies
	if (not checkNeedsRecompile(_file, outputFile)) {
		return;
	}

	// generating buildModeFlags
	auto flags          = _command.buildModeFlags;
	auto buildModeFlags = flags[buildModeName];

	// generate general flags
	auto strict = _project->getWarningsAsErrors()?_command.strict:std::vector<std::string>{};

	std::map<std::string, std::vector<std::string>> subMap;
	subMap["%compiler%"]       = {_command.searchPaths.back()};
	subMap["%infile%"]         = {_file};
	subMap["%outfile%"]        = {outputFile};
	subMap["%buildModeFlags%"] = buildModeFlags;
	subMap["%strict%"]         = strict;
	subMap["%genDefines%"]     = generateDefines(_project);
	subMap["%genIncludes%"]    = generateIncludes(_project);
	if (not mGenerateDBEntries) {
		auto options = substitute(_command.call, subMap);

		if (runCmd(options)) {
			utils::convertDFileToDDFile(buildPath + "/" + _file + ".d", buildPath + "/" + _file + ".dd");
		}

		// marking _file as recompiled and mark _project to be recompiled
		markProjectAndFileAsRecompiled(_project, _file);
	} else {
		auto call = _command.call;
		auto iter = std::find(begin(call), end(call), "%compiler%");
		call.erase(begin(call), iter);
		auto options = substitute(call, subMap);
		auto file    = _file;

		auto command = options[0];
		for (size_t i(1); i < options.size(); ++i) {
			command += " " + options[i];
		}
		mDBEntries.push_back({file, command});
	}
}

bool CompileBatch::checkNeedsRecompile(std::string const& _file, std::string const& outputFile) {

	// Check file dependencies
	if (mGenerateDBEntries) {
		return true;
	}

	bool recompile = false;
	auto outputFileDate = utils::getFileModificationTime(outputFile);
	auto ddFile = buildPath + "/" + _file + ".dd";
	if (not utils::fileExists(ddFile)) {
		recompile = true;
	} else if (utils::getFileModificationTime(_file) > outputFileDate) {
		recompile = true;
	} else {
		for (auto const& file : getDependenciesFromFile(ddFile)) {
			if (not utils::fileExists(file)) {
				recompile = true;
				break;
			}
			if (utils::getFileModificationTime(file) > outputFileDate) {
				recompile = true;
				break;
			}
		}
	}
	return recompile;
}
void CompileBatch::markProjectAndFileAsRecompiled(Project const* _project, std::string const& _file) {
	std::lock_guard<std::mutex> lock(printMutex);
	needsRecompile.insert(_project);
	fileWasRecompiled.insert(_file);
}


auto CompileBatch::generateDefines(Project const* _project) -> std::vector<std::string> {
	std::vector<std::string> options;

	options.push_back("-isystem.busy/helper-include");

	options.push_back("-DBUSY=BUSY");
	{
		std::string busyString = "BUSY_" + utils::sanitizeForMakro(_project->getName());
		options.push_back("-D" + busyString + "=" + busyString);
	}
	for (auto depP : _project->getDependenciesRecursive(ignoreProjects)) {
		std::string busyString = "BUSY_" + utils::sanitizeForMakro(depP->getName());
		options.push_back("-D" + busyString + "=" + busyString);
	}

	return options;
}
auto CompileBatch::generateIncludes(Project const* _project) -> std::vector<std::string> {
	std::vector<std::string> options;
	for (auto path : _project->getIncludeAndDependendPaths()) {
		options.push_back("-I" + path);
	}
	for (auto path : _project->getSystemIncludeAndDependendPaths()) {
		options.push_back("-isystem" + path);
	}
	for (auto const& p : toolchain->cCompiler.flags2) {
		options.push_back(p);
	}
	for (auto path : _project->getLegacySystemIncludeAndDependendPaths()) {
		options.push_back("-isystem" + path);
	}
	return options;
}

auto CompileBatch::objectFilesForLinking(Project const* _project, std::string const& _buildPath) const -> std::vector<std::string> {
	std::vector<std::string> options;
	for (auto file : _project->getCppFiles()) {
		auto objFile = _buildPath + "/" + file + ".o";
		options.push_back(objFile);
	}
	for (auto file : _project->getCFiles()) {
		auto objFile = _buildPath + "/" + file + ".o";
		options.push_back(objFile);
	}
	return options;
}

bool CompileBatch::runCmd(std::vector<std::string> const& _options) {
	// print command if verbose is activated
	if (verbose || mDryRun) {
		std::lock_guard<std::mutex> lock(printMutex);
		std::cout << std::endl;
		for (auto const& o : _options) {
			std::cout << o << " ";
		}
		std::cout << std::endl;
	}

	if (not mDryRun) {
		process::Process proc(_options);
		bool compileError = proc.getStatus() != 0;
		if (compileError) {
			printError(_options, proc);
			return false;
		}
	}
	return true;
}

void CompileBatch::printError(std::vector<std::string> const& _options, process::Process const& _proc) {
	if (not errorDetected) {
		std::lock_guard<std::mutex> lock(printMutex);
		if (not errorDetected) {
			errorDetected = true;

			std::cout << std::endl;
			for (auto const& o : _options) {
				std::cout << o << " ";
			}
			std::cout << std::endl;

			std::cout << _proc.cout() << std::endl;
			std::cerr << _proc.cerr() << std::endl;
		}
	}
}



}
}
