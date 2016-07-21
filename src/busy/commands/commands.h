#pragma once

#include "Workspace.h"

#include <memory>
#include <stdio.h>
#include <future>
#include <process/Process.h>

#include "utils.h"
#include "git.h"
#include "estd.h"

namespace commands {

	using namespace busy;

	bool build(std::string const& rootProjectName, bool verbose = false, bool noconsole=false, int jobs = 10);
	void clang();
	void clean();
	void cleanAll();
	void docu();
	void eclipse();
	void git(std::vector<std::string> const& _options);
	void test();
	void pull();
	void push();
	void install();
	void status(std::string _buildMode = "");
	void quickFix();
	int  listFiles(std::string const& _relPath);
	void showDep(std::string const& rootProjectName);
	void toolchains();
	void toolchain(std::string const& _toolchain);


	auto getAllToolchains(Workspace const& ws) -> std::map<std::string, Toolchain>;
	auto getAllFlavors(Workspace const& ws) -> std::map<std::string, Flavor>;

	auto getAllInstallations(Workspace const& ws) -> std::map<std::string, Installation>;
	void cloneMissingPackages  (Workspace const& ws);
	void checkingNotNeededPackages(Workspace& ws);
	void checkingInvalidPackages  (Workspace& ws);
	void checkingRequiredPackages (Workspace& ws);
}
