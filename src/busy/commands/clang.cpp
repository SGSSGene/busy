#include "commands.h"
#include <busyUtils/busyUtils.h>
#include <fstream>
#include <iostream>
#include <mutex>

#include "CompileBatch.h"
#include "Workspace.h"

namespace busy {
namespace commands {

void genClangdb() {
	Workspace ws;

	auto const toolchainName = ws.getSelectedToolchain();
	auto const buildModeName = ws.getSelectedBuildMode();

	auto const ignoreProjects = ws.getExcludedProjects(toolchainName);

	std::string const buildPath = ".busy/" + toolchainName + "/" + buildModeName;
	std::string const outPath   = "build/" + toolchainName + "/" + buildModeName;

	std::cout << "Using buildMode: " << buildModeName << std::endl;
	std::cout << "Using toolchain: " << toolchainName << std::endl;

	auto const toolchain = ws.getToolchains().at(toolchainName);

	Visitor visitor(ws, "");

	bool errorDetected {false};
	std::mutex printMutex;

	CompileBatch compileBatch(errorDetected, printMutex, buildPath, outPath, buildModeName, false, toolchain, ignoreProjects, false, false, true);
	compileBatch.mRPaths = ws.getRPaths();

	visitor.setCppVisitor([&] (Project const* _project, std::string const& _file) {
		compileBatch.compileCpp(_project, _file);
	});

	visitor.setCVisitor([&] (Project const* _project, std::string const& _file) {
		compileBatch.compileC(_project, _file);
	});
	visitor.setHVisitor([&] (Project const* _project, std::string const& _file) {
		compileBatch.compileCpp(_project, _file);
	});

	visitor.setProjectVisitor([&] (Project const* _project) {
		if (errorDetected) return;
		if (_project->getIsSingleFileProjects()) return;

		switch (_project->getType()) {
		case Project::Type::StaticLibrary:
			compileBatch.linkStaticLibrary(_project);
			break;
		case Project::Type::SharedLibrary:
			compileBatch.linkSharedLibrary(_project);
			break;
		case Project::Type::Plugin:
			compileBatch.linkPlugin(_project);
			break;
		case Project::Type::Executable:
			compileBatch.linkExecutable(_project);
			break;
		}
	});

	visitor.visit(1, true);

	auto cwd = utils::cwd();
	std::ofstream ofs("compile_commands.json");
	ofs << "[\n";

	auto const& entries = compileBatch.getDBEntries();
	if (not entries.empty()) {
		auto const& e = entries[0];
		ofs << "  { \"directory\": \"" << cwd       << "\",\n";
		ofs << "    \"command\":   \"" << e.command << "\",\n";
		ofs << "    \"file\":      \"" << e.file    << "\"\n";
		ofs << "  }";
	}

	for (size_t i = 1; i < entries.size(); ++i) {
		auto const& e = entries[i];
		ofs << ",\n";
		ofs << "  { \"directory\": \"" << cwd       << "\",\n";
		ofs << "    \"command\":   \"" << e.command << "\",\n";
		ofs << "    \"file\":      \"" << e.file    << "\"\n";
		ofs << "  }";
	}

	ofs << "\n]";
	ofs.close();
	std::cout << "generated compile_commands.json" << std::endl;
}

}
}
