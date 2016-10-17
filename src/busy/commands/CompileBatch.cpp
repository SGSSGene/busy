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
	compile(_project, _file, toolchain->cppCompiler.command);
}

void CompileBatch::compileC(Project const* _project, std::string const& _file) {
	compile(_project, _file, toolchain->cCompiler.command);
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

	std::vector<std::string> options = toolchain->archivist.command;
	options.push_back("rcs");

	options.push_back(buildPath + "/" + _project->getFullName() + ".a");

	for (auto file : objectFilesForLinking(_project, buildPath)) {
		options.push_back(file);
	}

	for (auto const& p : toolchain->archivist.postOptions) {
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
	std::string outputFile = buildPath + "/" + _project->getFullName("lib") + ".so";

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
		} else {
			for (auto _project : _project->getDependenciesRecursiveOnlyStaticNotOverShared(ignoreProjects)) {
				if (needsRecompile.count(_project) > 0) {
					recompile = true;
					break;
				}
			}
		}
		if (not recompile) {
			return;
		}
	}


	std::vector<std::string> options = toolchain->cppCompiler.command;
	//!TODO shouldnt be default argument
	options.push_back("-rdynamic");
	options.push_back("-shared");
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
	// add shared libraries
	for (auto project : _project->getDependenciesRecursiveOnlyShared(ignoreProjects)) {
		if (project->getIsHeaderOnly()) continue;

		std::string fullPath = buildPath + "/" + project->getFullName();
		fullPath = fullPath.substr(0, fullPath.find_last_of("/"));

		options.push_back("-L");
		options.push_back(fullPath);
		options.push_back("-l"+project->getName());
	}


	for (auto dep : _project->getSystemLibraries()) {
		options.push_back("-l"+dep);
	}
	for (auto project : _project->getDependenciesRecursiveOnlyStaticNotOverShared(ignoreProjects)) {
		for (auto dep : project->getSystemLibraries()) {
			options.push_back("-l"+dep);
		}
	}
/*	for (auto project : _project->getDependenciesRecursiveOnlyShared(ignoreProjects)) {
		for (auto dep : project->getSystemLibraries()) {
			options.push_back("-l"+dep);
		}
	}*/

	for (auto const& p : toolchain->cppCompiler.postOptions) {
		options.push_back(p);
	}
	for (auto linking : _project->getLinkingOptionsRecursive()) {
		options.push_back(linking);
	}

	for (auto project : _project->getDependenciesRecursiveOnlyStaticNotOverShared(ignoreProjects)) {
		for (auto linking : project->getSystemLibrariesPaths()) {
			options.push_back("-L");
			options.push_back(linking);
		}
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
		if (not recompile) {
			return;
		} else {
			for (auto _project : _project->getDependenciesRecursive(ignoreProjects)) {
				if (needsRecompile.count(_project) > 0) {
					recompile = true;
					break;
				}
			}
		}
		if (not recompile) {
			return;
		}
	}


	std::vector<std::string> options = toolchain->cppCompiler.command;
	//!TODO shouldnt be default argument
	options.push_back("-rdynamic");
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
	// add shared libraries
	for (auto project : _project->getDependenciesRecursiveOnlyShared(ignoreProjects)) {
		if (project->getIsHeaderOnly()) continue;

		std::string fullPath = buildPath + "/" + project->getFullName();
		fullPath = fullPath.substr(0, fullPath.find_last_of("/"));

		options.push_back("-L");
		options.push_back(fullPath);
		options.push_back("-l"+project->getName());
	}

	for (auto dep : _project->getSystemLibraries()) {
		options.push_back("-l"+dep);
	}
	for (auto project : _project->getDependenciesRecursiveOnlyStaticNotOverShared(ignoreProjects)) {
		for (auto dep : project->getSystemLibraries()) {
			options.push_back("-l"+dep);
		}
	}
/*	for (auto project : _project->getDependenciesRecursiveOnlyShared(ignoreProjects)) {
		for (auto dep : project->getSystemLibraries()) {
			options.push_back("-l"+dep);
		}
	}*/
	for (auto const& p : toolchain->cppCompiler.postOptions) {
		options.push_back(p);
	}
	for (auto linking : _project->getLinkingOptionsRecursive()) {
		options.push_back(linking);
	}

	for (auto project : _project->getDependenciesRecursiveOnlyStaticNotOverShared(ignoreProjects)) {
		for (auto linking : project->getSystemLibrariesPaths()) {
			options.push_back("-L");
			options.push_back(linking);
		}
	}


	printVerboseCmd(options);

	process::Process proc(options);

	bool compileError = proc.getStatus() != 0;
	if (compileError) {
		printError(options, proc);
	}
}

void CompileBatch::compile(Project const* _project, std::string const& _file, std::vector<std::string> _options) {
	if (errorDetected) return;

	std::string outputFile = buildPath + "/" + _file + ".o";

	// Check file dependencies
	if (not checkNeedsRecompile(_file, outputFile)) {
		return;
	}

	// marking _file as recompiled and mark _project to be recompiled
	insertProjectAndFile(_project, _file);

	for (auto& o : generateFlags(_project, _file, outputFile)) {
		_options.push_back(o);
	}
	for (auto& d : generateDefines(_project)) {
		_options.emplace_back(std::move(d));
	}
	for (auto& i : generateIncludes(_project)) {
		_options.emplace_back(std::move(i));
	}
	compile(_options, _file);
}




bool CompileBatch::checkNeedsRecompile(std::string const& _file, std::string const& outputFile) {
	// Check file dependencies
	bool recompile = false;
	if (not utils::fileExists(buildPath + "/" + _file + ".dd")) {
		recompile = true;
	} else for (auto const& file : getDependenciesFromFile(buildPath + "/" + _file + ".dd")) {
		auto outputFileDate = utils::getFileModificationTime(outputFile);
		if (not utils::fileExists(file)) {
			recompile = true;
			break;
		}
		if (utils::getFileModificationTime(file) > outputFileDate) {
			recompile = true;
			break;
		}
	}
	return recompile;
}
void CompileBatch::insertProjectAndFile(Project const* _project, std::string const& _file) {
	std::lock_guard<std::mutex> lock(printMutex);
	needsRecompile.insert(_project);
	fileWasRecompiled.insert(_file);
}


auto CompileBatch::generateFlags(Project const* _project, std::string const& _file, std::string const& outputFile) -> std::vector<std::string> {
	std::vector<std::string> options;

	//!TODO shouldnt be default argument
	options.push_back("-c");
	options.push_back(_file);
	options.push_back("-o");
	options.push_back(outputFile);
	if (buildModeName == "release") {
		options.push_back("-O3");
	} else if (buildModeName == "release_with_symbols") {
		options.push_back("-O3");
		options.push_back("-g3");
	} else if (buildModeName == "debug") {
		options.push_back("-g3");
		options.push_back("-O0");
	}
	//!ENDTODO
	return options;
}


auto CompileBatch::generateDefines(Project const* _project) -> std::vector<std::string> {
	std::vector<std::string> options;

	options.push_back("-isystem");
	options.push_back(".busy/helper-include");

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
		options.push_back("-I");
		options.push_back(path);
	}
	for (auto path : _project->getSystemIncludeAndDependendPaths()) {
		options.push_back("-isystem");
		options.push_back(path);
	}
	for (auto const& p : toolchain->cCompiler.postOptions) {
		options.push_back(p);
	}
	for (auto path : _project->getLegacySystemIncludeAndDependendPaths()) {
		options.push_back("-isystem");
		options.push_back(path);
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
