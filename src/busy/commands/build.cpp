#include "commands.h"

#include "Workspace.h"
#include <busyUtils/busyUtils.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <process/Process.h>

#define TERM_RED                        "\033[31m"
#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"

using namespace busy;

static auto convertStaticToShared(std::string _shared) -> std::string {
	// adding "lib" into the name
	{
		auto pos = _shared.find_last_of('/') + 1;
		auto part0 = _shared.substr(0, pos);
		auto part1 = _shared.substr(pos);
		_shared = part0 + "lib" + part1;
	}
	// replacing .a with .so ending
	{
		_shared.pop_back(); _shared.pop_back();
		_shared = _shared + "so";
	}
	return _shared;
}

namespace commands {

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

bool build(std::string const& rootProjectName, bool verbose, bool noconsole, int jobs) {

	std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();
	Workspace ws;

	auto toolchainName = ws.getSelectedToolchain();
	auto buildModeName = ws.getSelectedBuildMode();

	std::string buildPath = ".busy/" + toolchainName + "/" + buildModeName;
	std::string outPath   = "build/" + toolchainName + "/" + buildModeName;

	std::cout << "Using buildMode: " << buildModeName << std::endl;
	std::cout << "Using toolchain: " << toolchainName << std::endl;

	auto toolchain = ws.getToolchains().at(toolchainName);

	Visitor visitor(ws, rootProjectName);
	std::mutex printMutex;

	bool errorDetected = false;

	// create all needed path
	{
		std::set<std::string> neededPath;
		for (auto const& project : ws.getProjectAndDependencies(rootProjectName)) {
			// paths for .o and .d files
			for (auto file : project->getCppFiles()) {
				neededPath.insert(utils::dirname(buildPath + "/" + file));
			}
			for (auto file : project->getCFiles()) {
				neededPath.insert(utils::dirname(buildPath + "/" + file));
			}

			// paths for .a files
			neededPath.insert(utils::dirname(buildPath + "/" + project->getFullName()));

			// paths needed for .a of singleFileProjects
			for (auto f : project->getCppAndCFiles()) {
				neededPath.insert(utils::dirname(buildPath + "/" + f));
			}

			// paths for executables
			std::string outputFile = outPath + "/" + project->getName();
			if (project->getIsUnitTest()) {
				outputFile = outPath + "/tests/" + project->getFullName();
			} else if (project->getIsExample()) {
				outputFile = outPath + "/examples/" + project->getFullName();
			}
			neededPath.insert(utils::dirname(outputFile));
		}

		for (auto const& s : neededPath) {
			if (not utils::fileExists(s)) {
				utils::mkdir(s);
			}
		}
	}

	visitor.setStatisticUpdateCallback([&] (int done, int total) {
		std::lock_guard<std::mutex> lock(printMutex);
		if (errorDetected) return;
		if (not noconsole) {
			std::cout << "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";
			std::cout << "working on job: " << done << "/" << total << std::flush;

			if (done == total) {
				std::cout << std::endl;
			}
		} else if (done == total) {
			std::cout << "working on job: "<< done << "/" << total << std::endl;
		}

	});

	std::set<Project const*> needsRecompile;
	std::set<std::string>    fileWasRecompiled;
	visitor.setCppVisitor([&] (Project const* _project, std::string const& _file) {
		if (errorDetected) return;
		std::string outputFile = buildPath + "/" + _file + ".o";

		// Check file dependencies
		bool recompile = false;
		if (not utils::fileExists(buildPath + "/" + _file + ".dd")) {
			recompile = true;
		} else {
			auto outputFileDate = utils::getFileModificationTime(outputFile);
			for (auto const& file : getDependenciesFromFile(buildPath + "/" + _file + ".dd")) {
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
		if (not recompile) {
			return;
		}
		{
			std::lock_guard<std::mutex> lock(printMutex);
			needsRecompile.insert(_project);
			fileWasRecompiled.insert(_file);
		}

		std::vector<std::string> options = toolchain->cppCompiler;
		options.push_back("-std=c++11");
		//!TODO shouldnt be default argument
		options.push_back("-Wall");
		options.push_back("-Wextra");
		options.push_back("-fmessage-length=0");
		options.push_back("-fPIC");
		options.push_back("-rdynamic");
		options.push_back("-fmax-errors=3");
		options.push_back("-MD");
		options.push_back("-c");
		options.push_back(_file);
		options.push_back("-o");
		options.push_back(outputFile);
		options.push_back("-g3");
		options.push_back("-O0");
		//!ENDTODO
		options.push_back("-DBUSY");
		options.push_back("-DBUSY_" + utils::sanitizeForMakro(_project->getName()));
		for (auto depP : _project->getDependenciesRecursive()) {
			options.push_back("-DBUSY_" + utils::sanitizeForMakro(depP->getName()));
		}
		for (auto path : _project->getIncludeAndDependendPaths()) {
			options.push_back("-I");
			options.push_back(path);
		}
		for (auto path : _project->getSystemIncludeAndDependendPaths()) {
			options.push_back("-isystem");
			options.push_back(path);
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
		if (not compileError) {
			utils::convertDFileToDDFile(buildPath + "/" + _file + ".d", buildPath + "/" + _file + ".dd");
		} else {
			if (not errorDetected) {
				std::lock_guard<std::mutex> lock(printMutex);
				errorDetected = true;

				std::cout << std::endl;
				for (auto const& o : options) {
					std::cout << o << " ";
				}
				std::cout << std::endl;

				std::cout << proc.cout() << std::endl;
				std::cerr << proc.cerr() << std::endl;
			}
		}
	});

	visitor.setCVisitor([&] (Project const* _project, std::string const& _file) {
		if (errorDetected) return;

		std::string outputFile = buildPath + "/" + _file + ".o";

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
		if (not recompile) {
			return;
		}
		{
			std::lock_guard<std::mutex> lock(printMutex);
			needsRecompile.insert(_project);
			fileWasRecompiled.insert(_file);
		}

		std::vector<std::string> options = toolchain->cCompiler;
		options.push_back("-std=c11");
		//!TODO shouldnt be default argument
		options.push_back("-Wall");
		options.push_back("-Wextra");
		options.push_back("-fmessage-length=0");
		options.push_back("-fPIC");
		options.push_back("-rdynamic");
		options.push_back("-fmax-errors=3");
		options.push_back("-MD");
		options.push_back("-c");
		options.push_back(_file);
		options.push_back("-o");
		options.push_back(outputFile);
		if (buildModeName == "release") {
			options.push_back("-O3");
		} else if (buildModeName == "debug") {
			options.push_back("-g3");
			options.push_back("-O0");
		}
		//!ENDTODO
		options.push_back("-DBUSY");
		options.push_back("-DBUSY_" + utils::sanitizeForMakro(_project->getName()));
		for (auto depP : _project->getDependenciesRecursive()) {
			options.push_back("-DBUSY_" + utils::sanitizeForMakro(depP->getName()));
		}
		for (auto path : _project->getIncludeAndDependendPaths()) {
			options.push_back("-I");
			options.push_back(path);
		}
		for (auto path : _project->getSystemIncludeAndDependendPaths()) {
			options.push_back("-isystem");
			options.push_back(path);
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
		if (proc.getStatus() == 0) {
			utils::convertDFileToDDFile(buildPath + "/" + _file + ".d", buildPath + "/" + _file + ".dd");
		} else {
			std::lock_guard<std::mutex> lock(printMutex);
			if (not errorDetected) {
				errorDetected = true;

				std::cout << std::endl;
				for (auto const& o : options) {
					std::cout << o << " ";
				}
				std::cout << std::endl;

				std::cout << proc.cout() << std::endl;
				std::cerr << proc.cerr() << std::endl;
			}
		}
	});


	auto linkLibrary = [&] (Project const* _project) {
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

		std::vector<std::string> options = toolchain->archivist;
		options.push_back("rcs");

		options.push_back(buildPath + "/" + _project->getFullName() + ".a");
		for (auto file : _project->getCppFiles()) {
			auto objFile = buildPath + "/" + file + ".o";
			options.push_back(objFile);
		}
		for (auto file : _project->getCFiles()) {
			auto objFile = buildPath + "/" + file + ".o";
			options.push_back(objFile);
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
			std::lock_guard<std::mutex> lock(printMutex);
			if (not errorDetected) {
				errorDetected = true;

				std::cout << std::endl;
				for (auto const& o : options) {
					std::cout << o << " ";
				}
				std::cout << std::endl;

				std::cout << proc.cout() << std::endl;
				std::cerr << proc.cerr() << std::endl;
			}
		}
	};

	auto linkLibrarySingleFileProjects = [&] (Project const* _project) {

		auto files = _project->getCppFiles();
		{
			auto cFiles   = _project->getCFiles();
			for (auto& f : cFiles) {
				files.emplace_back(std::move(f));
			}
		}

		for (auto f : files) {
			std::string outputFile = buildPath + "/" + f + ".a";
			outputFile = convertStaticToShared(outputFile);
			// Check file dependencies
			bool recompile = false;
			{
				std::lock_guard<std::mutex> lock(printMutex);
				if (needsRecompile.count(_project) > 0 and fileWasRecompiled.count(f) > 0) {
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

			std::vector<std::string> options = toolchain->cppCompiler;
			options.push_back("-rdynamic");
			options.push_back("-shared");
			options.push_back("-o");
			options.push_back(outputFile);
			options.push_back(buildPath + "/" + f + ".o");
			process::Process proc(options);
			bool compileError = proc.getStatus() != 0;
			if (compileError) {
				std::lock_guard<std::mutex> lock(printMutex);
				if (not errorDetected) {
					errorDetected = true;

					std::cout << std::endl;
					for (auto const& o : options) {
						std::cout << o << " ";
					}
					std::cout << std::endl;

					std::cout << proc.cout() << std::endl;
					std::cerr << proc.cerr() << std::endl;
				}
			}
		}
	};

	auto linkExecutable = [&] (Project const* _project) {
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
			} else {
				for (auto project : _project->getDependenciesRecursive()) {
					if (needsRecompile.count(project) > 0) {
						recompile = true;
						break;
					}
				}
			}
			if (not recompile) {
				return;
			}
		}



		std::vector<std::string> options = toolchain->cppCompiler;
		//!TODO shouldnt be default argument
		options.push_back("-rdynamic");
		//!ENDTODO
		options.push_back("-o");
		options.push_back(outputFile);

		for (auto file : _project->getCppFiles()) {
			auto objFile = buildPath + "/" + file + ".o";
			options.push_back(objFile);
		}
		for (auto project : _project->getDependenciesRecursive()) {
			if (project->getIsHeaderOnly()) continue;

			if (not project->getIsSingleFileProjects()) {
				if (project->getWholeArchive()) {
					options.push_back("-Wl,--whole-archive");
				}
				options.push_back(buildPath + "/" + project->getFullName() + ".a");
				if (project->getWholeArchive()) {
					options.push_back("-Wl,--no-whole-archive");
				}
			} else {
/*				for (auto f : project->getCppAndCFiles()) {
					auto outputFile = buildPath + "/" + f + ".a";
					outputFile = convertStaticToShared(outputFile);
					options.push_back("-L");
					options.push_back(utils::dirname(outputFile));
				if (project->getWholeArchive()) {
					options.push_back("-Wl,--whole-archive");
				}
					options.push_back("-l" + utils::_basename(f));
				if (project->getWholeArchive()) {
					options.push_back("-Wl,--no-whole-archive");
				}

					options.push_back("-Wl,-rpath=" + utils::dirname(outputFile));
				}*/
			}
		}

		for (auto dep : _project->getSystemLibraries()) {
			options.push_back("-l"+dep);
		}
		for (auto project : _project->getDependenciesRecursive()) {
			for (auto dep : project->getSystemLibraries()) {
				options.push_back("-l"+dep);
			}
		}
		for (auto linking : _project->getSystemLibrariesPathsRecursive()) {
			options.push_back("-L");
			options.push_back(linking);
		}
		for (auto linking : _project->getLinkingOptionsRecursive()) {
			options.push_back(linking);
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
			std::lock_guard<std::mutex> lock(printMutex);
			if (not errorDetected) {
				errorDetected = true;

				std::cout << std::endl;
				for (auto const& o : options) {
					std::cout << o << " ";
				}
				std::cout << std::endl;

				std::cout << proc.cout() << std::endl;
				std::cerr << proc.cerr() << std::endl;
			}
		}
	};
	visitor.setProjectVisitor([&] (Project const* _project) {
		if (errorDetected) return;

		if (_project->getType() == "library") {
			if (not _project->getIsSingleFileProjects()) {
				linkLibrary(_project);
			} else {
				linkLibrarySingleFileProjects(_project);
			}
		} else if (_project->getType() == "executable") {
			linkExecutable(_project);
		} else {
			std::lock_guard<std::mutex> lock(printMutex);
			errorDetected = true;
			std::cout << "unknown type: " << _project->getType();
		}
	});

	visitor.visit(jobs);

	std::chrono::high_resolution_clock::time_point endTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(endTime - startTime);



	if (errorDetected) {
		std::cout<<std::endl<< TERM_RED "Build failed" TERM_RESET;
	} else {
		std::cout<<std::endl<< TERM_GREEN "Build \033[32msucceeded" TERM_RESET;
	}

	std::cout<< " after " << time_span.count() << " seconds." << std::endl;
	return not errorDetected;
}
}
