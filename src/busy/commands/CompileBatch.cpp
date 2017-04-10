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
	std::string outputFile = buildPath + "/" + _project->getFullName() + ".a";

	// Check file dependencies
	bool recompile = false;
	{
		std::lock_guard<std::mutex> lock(printMutex);
		if (needsRecompile.count(_project) > 0) {
			recompile = true;
		} else if (not utils::fileExists(outputFile)) {
			recompile = true;
		}
		if (not recompile) {
			return;
		}

		if (_project->getIsHeaderOnly()) return;
		needsRecompile.insert(_project);
	}

	std::vector<std::string> options;
	options.push_back(toolchain->archivist.searchPaths.back());
	for (auto const& o : toolchain->archivist.flags) {
		options.push_back(o);
	}

	options.push_back(buildPath + "/" + _project->getFullName() + ".a");

	for (auto file : objectFilesForLinking(_project, buildPath)) {
		options.push_back(file);
	}

	for (auto const& p : toolchain->archivist.flags2) {
		options.push_back(p);
	}

	if (verbose) {
		std::lock_guard<std::mutex> lock(printMutex);
		std::cout << std::endl;
		for (auto const& o : options) {
			std::cout << o << " ";
		}
		std::cout << std::endl;
	}

	process::Process proc(options);
	bool compileError = proc.getStatus() != 0;
	if (compileError) {
		printError(options, proc);
	}
}
void CompileBatch::linkSharedLibrary(Project const* _project) {
	std::string outputFile = outPath + "/lib" + _project->getName() + ".so";
	linkSharedLibraryImpl(_project, outputFile);
}
void CompileBatch::linkPlugin(Project const* _project) {
	std::string outputFile = outPath + "/plugin" + _project->getName() + ".so";
	linkSharedLibraryImpl(_project, outputFile);
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

void CompileBatch::linkSharedLibraryImpl(Project const* _project, std::string const& outputFile) {
	//std::string outputFile = buildPath + "/" + _project->getFullName("lib") + ".so";
	//std::string outputFile = outPath + "/lib" + _project->getName() + ".so";

	// Check file dependencies
	bool recompile = false;
	{
		std::lock_guard<std::mutex> lock(printMutex);
		if (needsRecompile.count(_project) > 0) {
			recompile = true;
		} else if (not utils::fileExists(outputFile)) {
			recompile = true;
		}

		for (auto _p : _project->getDependenciesRecursiveOnlyStaticNotOverShared(ignoreProjects)) {
//		for (auto _p : _project->getDependenciesRecursive(ignoreProjects)) {
			if (needsRecompile.count(_p) > 0) {
				recompile = true;
				break;
			}
		}
		if (not recompile) {
			return;
		}
	}


	std::vector<std::string> options;
	options.push_back(toolchain->linkExecutable.searchPaths.back());
	for (auto const& o : toolchain->linkExecutable.flags) {
		options.push_back(o);
	}
	//!TODO shouldnt be default argument
//	std::cout << "linking shared: " << outputFile << "\n";
	options.push_back("-shared");

	char const* ld = getenv("BUSY_LD");
	if (ld != nullptr) {
		options.push_back(std::string("-fuse-ld=") + ld);
	}

	//!ENDTODO
	options.push_back("-o");
	options.push_back(outputFile);

	for (auto file : objectFilesForLinking(_project, buildPath)) {
		options.push_back(file);
	}

	// add static libraries
	for (auto project : _project->getDependenciesRecursiveOnlyStaticNotOverShared(ignoreProjects)) {
		if (project->getIsHeaderOnly()) continue;
		if (true) {
//		if (project->getWholeArchive()) {
			options.push_back("-Wl,--whole-archive");
		}
		options.push_back(buildPath + "/" + project->getFullName() + ".a");
		if (true) {
//		if (project->getWholeArchive()) {
			options.push_back("-Wl,--no-whole-archive");
		}
	}
	std::vector<std::string> systemLibraries;
	std::vector<std::string> systemLibrariesPaths;
	// add shared libraries
	for (auto project : _project->getDependenciesRecursiveOnlyShared(ignoreProjects)) {
		if (project->getIsHeaderOnly()) continue;

//		std::string fullPath = buildPath + "/" + project->getFullName();
//		fullPath = fullPath.substr(0, fullPath.find_last_of("/"));
		std::string fullPath = outPath + "/";


		systemLibrariesPaths.push_back(fullPath);
		systemLibraries.push_back("-l"+project->getName());
	}


	for (auto dep : _project->getSystemLibraries()) {
		systemLibraries.push_back("-l"+dep);
	}
	for (auto project : _project->getDependenciesRecursiveOnlyStaticNotOverShared(ignoreProjects)) {
		for (auto dep : project->getSystemLibraries()) {
			systemLibraries.push_back("-l"+dep);
		}
	}
/*	for (auto project : _project->getDependenciesRecursiveOnlyShared(ignoreProjects)) {
		for (auto dep : project->getSystemLibraries()) {
			options.push_back("-l"+dep);
		}
	}*/

	for (auto const& p : toolchain->cppCompiler.flags2) {
		options.push_back(p);
	}
	for (auto linking : _project->getLinkingOptionsRecursive()) {
		systemLibraries.push_back(linking);
	}

	for (auto project : _project->getDependenciesRecursiveOnlyStaticNotOverShared(ignoreProjects)) {
		for (auto linking : project->getSystemLibrariesPaths()) {
			systemLibrariesPaths.push_back(linking);
		}
	}

	systemLibrariesPaths = removeLastDuplicates(systemLibrariesPaths);
	for (auto const& s : systemLibrariesPaths) {
		options.push_back("-L"+s);
	}

	systemLibraries = removeLastDuplicates(systemLibraries);
	for (auto const& s : systemLibraries) {
		options.push_back(s);
	}

	printVerboseCmd(options);

	process::Process proc(options);

	bool compileError = proc.getStatus() != 0;
	if (compileError) {
		printError(options, proc);
	}
}

void CompileBatch::linkExecutable(Project const* _project) {
	std::string outputFile = outPath + "/" + _project->getName();
	if (_project->getIsUnitTest()) {
		outputFile = outPath + "/tests/" + _project->getFullName();
	} else if (_project->getIsExample()) {
		outputFile = outPath + "/examples/" + _project->getFullName();
	}

	// Check file dependencies
	bool recompile = false;
	{
		std::lock_guard<std::mutex> lock(printMutex);
		if (needsRecompile.count(_project) > 0) {
			recompile = true;
		} else if (not utils::fileExists(outputFile)) {
			recompile = true;
		}
		for (auto _p : _project->getDependenciesRecursiveOnlyStaticNotOverShared(ignoreProjects)) {
//		for (auto _p : _project->getDependenciesRecursive(ignoreProjects)) {
			if (needsRecompile.count(_p) > 0) {
				recompile = true;
				break;
			}
		}
		if (not recompile) {
			return;
		}
	}


	std::vector<std::string> options;
	options.push_back(toolchain->linkExecutable.searchPaths.back());
	for (auto const& o : toolchain->linkExecutable.flags) {
		options.push_back(o);
	}

	//!TODO shouldnt be default argument
	char const* ld = getenv("BUSY_LD");
	if (ld != nullptr) {
		options.push_back(std::string("-fuse-ld=") + ld);
	}

	//!ENDTODO
	options.push_back("-o");
	options.push_back(outputFile);

	for (auto file : objectFilesForLinking(_project, buildPath)) {
		options.push_back(file);
	}

	// add static libraries
	for (auto project : _project->getDependenciesRecursiveOnlyStaticNotOverShared(ignoreProjects)) {
		if (project->getIsHeaderOnly()) continue;

		if (project->getWholeArchive()) {
			options.push_back("-Wl,--whole-archive");
		}
		options.push_back(buildPath + "/" + project->getFullName() + ".a");
		if (project->getWholeArchive()) {
			options.push_back("-Wl,--no-whole-archive");
		}
	}

	std::vector<std::string> systemLibraries;
	std::vector<std::string> systemLibrariesPaths;
	// add shared libraries
	for (auto project : _project->getDependenciesRecursiveOnlyShared(ignoreProjects)) {
		if (project->getIsHeaderOnly()) continue;

		std::string fullPath = outPath + "/";

		systemLibrariesPaths.push_back(fullPath);
		systemLibraries.push_back("-l"+project->getName());
	}

	for (auto const& r : mRPaths) {
		options.push_back("-Wl,-rpath");
		options.push_back(r);
	}

	for (auto dep : _project->getSystemLibraries()) {
		systemLibraries.push_back("-l"+dep);
	}
	//for (auto project : _project->getDependenciesRecursiveOnlyStaticNotOverShared(ignoreProjects)) {
	for (auto project : _project->getDependenciesRecursive()) {
		for (auto dep : project->getSystemLibraries()) {
			systemLibraries.push_back("-l"+dep);
		}
	}
/*	for (auto project : _project->getDependenciesRecursiveOnlyShared(ignoreProjects)) {
		for (auto dep : project->getSystemLibraries()) {
			options.push_back("-l"+dep);
		}
	}*/
	for (auto const& p : toolchain->cppCompiler.flags2) {
		options.push_back(p);
	}
	for (auto linking : _project->getLinkingOptionsRecursive()) {
		options.push_back(linking);
	}

	for (auto project : _project->getDependenciesRecursive()) {
//	for (auto project : _project->getDependenciesRecursiveOnlyStatic(ignoreProjects)) {
		for (auto linking : project->getSystemLibrariesPaths()) {
			systemLibrariesPaths.push_back(linking);
		}
	}

	systemLibrariesPaths = removeLastDuplicates(systemLibrariesPaths);
	for (auto const& s : systemLibrariesPaths) {
		options.push_back("-L");
		options.push_back(s);
	}

	systemLibraries = removeLastDuplicates(systemLibraries);
	for (auto const& s : systemLibraries) {
		options.push_back(s);
	}

	printVerboseCmd(options);

	process::Process proc(options);

	bool compileError = proc.getStatus() != 0;
	if (compileError) {
		printError(options, proc);
	}
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
	std::vector<std::string> buildModeFlags;
	auto iter = _command.buildModeFlags.find(buildModeName);
	if (iter != _command.buildModeFlags.end()) {
		buildModeFlags = iter->second;
	}

	// generate general flags
	std::vector<std::string> options;
	options.push_back(_command.searchPaths.back());
	if (_project->getWarningsAsErrors()) {
		for (auto const& o :_command.strict) {
			options.push_back(o);
		}
	}
	for (auto const& o : _command.flags) {
		if (o == "%infile%") {
			options.push_back(_file);
		} else if (o == "%outfile%") {
			options.push_back(outputFile);
		} else if (o == "%buildModeFlags%") {
			for (auto const& f : buildModeFlags) {
				options.push_back(f);
			}
		} else {
			options.push_back(o);
		}
	}


	// marking _file as recompiled and mark _project to be recompiled
	insertProjectAndFile(_project, _file);

	for (auto& d : generateDefines(_project)) {
		options.emplace_back(std::move(d));
	}
	for (auto& i : generateIncludes(_project)) {
		options.emplace_back(std::move(i));
	}
	compile(options, _file);
}




bool CompileBatch::checkNeedsRecompile(std::string const& _file, std::string const& outputFile) {

	// Check file dependencies
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
void CompileBatch::insertProjectAndFile(Project const* _project, std::string const& _file) {
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

void CompileBatch::compile(std::vector<std::string> const& _options, std::string const& _file) {
	printVerboseCmd(_options);

	// run actuall command and do dependency file conversion
	process::Process proc(_options);
	bool compileError = proc.getStatus() != 0;
	if (not compileError) {
		utils::convertDFileToDDFile(buildPath + "/" + _file + ".d", buildPath + "/" + _file + ".dd");
	} else {
		printError(_options, proc);
	}
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
void CompileBatch::printVerboseCmd(std::vector<std::string> const& _options) {
	// print command if verbose is activated
	if (verbose) {
		std::lock_guard<std::mutex> lock(printMutex);
		std::cout << std::endl;
		for (auto const& o : _options) {
			std::cout << o << " ";
		}
		std::cout << std::endl;
	}

}

}
}
