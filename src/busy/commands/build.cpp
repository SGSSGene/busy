#include "commands.h"

#include <chrono>
#include <iostream>
#include <fstream>

#include <serializer/serializer.h>
#include <process/Process.h>

#include "NeoWorkspace.h"

#include "BuildAction.h"
#include "FileStates.h"

#define TERM_RED                        "\033[31m"
#define TERM_GREEN                      "\033[32m"
#define TERM_RESET                      "\033[0m"


using namespace busy;

namespace commands {

bool build(std::string const& rootProjectName, bool verbose, bool noconsole, int jobs) {
	NeoWorkspace ws;

	std::string toolchainName = "system-gcc";
	auto toolchain = ws.getToolchains().at(toolchainName);


	NeoVisitor visitor(ws, rootProjectName);
	std::mutex printMutex;

	visitor.setCppVisitor([toolchain, &printMutex, &toolchainName, verbose] (NeoProject const* _project, std::string const& _file) {
		std::string buildPath = ".busy/neo/" + toolchainName;
		std::string buildFilePath = utils::dirname(buildPath + "/" + _file);
		utils::mkdir(buildFilePath);
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
		options.push_back(buildPath + "/" + _file+".o");
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
			std::lock_guard<std::mutex> lock(printMutex);
			for (auto const& o : options) {
				std::cout << o << " ";
			}
			std::cout << std::endl;

			std::cout << proc.cout() << std::endl;
			std::cerr << proc.cerr() << std::endl;
			std::cout << "status: " << proc.getStatus() << std::endl;
		}
	});

	visitor.setCVisitor([toolchain, &printMutex, &toolchainName, verbose] (NeoProject const* _project, std::string const& _file) {
		std::string buildPath = ".busy/neo/" + toolchainName;
		std::string buildFilePath = utils::dirname(buildPath + "/" + _file);
		utils::mkdir(buildFilePath);
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
		options.push_back(buildPath + "/" + _file+".o");
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
		process::Process proc(options);
		std::lock_guard<std::mutex> lock(printMutex);
		if (proc.getStatus() == 0) {
			utils::convertDFileToDDFile(buildPath + "/" + _file + ".d", buildPath + "/" + _file + ".dd");
		} else {
			for (auto const& o : options) {
				std::cout << o << " ";
			}
			std::cout << std::endl;
			std::cout << "status: " << proc.getStatus() << std::endl;

			std::cout << proc.cout() << std::endl;
			std::cerr << proc.cerr() << std::endl;
		}
	});


	auto linkLibrary = [toolchain, &printMutex, &toolchainName, verbose] (NeoProject const* _project) {
		std::vector<std::string> options = toolchain->archivist;
		options.push_back("rcs");

		std::string buildPath = ".busy/neo/" + toolchainName;
		std::string buildFilePath = utils::dirname(buildPath + "/" + _project->getFullName() + ".a");
		utils::mkdir(buildFilePath);
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
			for (auto const& o : options) {
				std::cout << o << " ";
			}
			std::cout << std::endl;
		}

		process::Process proc(options);

		bool compileError = proc.getStatus() != 0;
		if (compileError) {
			std::lock_guard<std::mutex> lock(printMutex);

			for (auto const& o : options) {
				std::cout << o << " ";
			}
			std::cout << std::endl;
			std::cout << "status: " << proc.getStatus() << std::endl;

			std::cout << proc.cout() << std::endl;
			std::cerr << proc.cerr() << std::endl;
		}
	};
	auto linkExecutable = [toolchain, &printMutex, &toolchainName, verbose] (NeoProject const* _project) {
		std::string buildPath = ".busy/neo/" + toolchainName;
		std::string buildFilePath = utils::dirname(buildPath + "/" + _project->getFullName());
		utils::mkdir(buildFilePath);

		std::vector<std::string> options = toolchain->cppCompiler;
		//!TODO shouldnt be default argument
		options.push_back("-rdynamic");
		//!ENDTODO
		options.push_back("-o");
		options.push_back(buildPath + "/" + _project->getFullName());
		for (auto file : _project->getCppFiles()) {
			auto objFile = buildPath + "/" + file + ".o";
			options.push_back(objFile);
		}
		for (auto project : _project->getDependenciesRecursive()) {
			if (project->getWholeArchive()) {
				options.push_back("-Wl,--whole-archive");
			}
			options.push_back(buildPath + "/" + project->getFullName() + ".a");
			if (project->getWholeArchive()) {
				options.push_back("-Wl,--no-whole-archive");
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
			for (auto const& o : options) {
				std::cout << o << " ";
			}
			std::cout << std::endl;
		}

		process::Process proc(options);

		bool compileError = proc.getStatus() != 0;
		if (compileError) {
			std::lock_guard<std::mutex> lock(printMutex);
			for (auto const& o : options) {
				std::cout << o << " ";
			}
			std::cout << std::endl;

			std::cout << "status: " << proc.getStatus() << std::endl;

			std::cout << proc.cout() << std::endl;
			std::cerr << proc.cerr() << std::endl;
		}
	};
	visitor.setProjectVisitor([linkLibrary, linkExecutable, &printMutex] (NeoProject const* _project) {
		if (_project->getType() == "library") {
			linkLibrary(_project);
		} else if (_project->getType() == "executable") {
			linkExecutable(_project);
		} else {
			std::lock_guard<std::mutex> lock(printMutex);
			std::cout << "unknown type: " << _project->getType();
		}
	});

	visitor.visit(jobs);
	return true;
}
}
