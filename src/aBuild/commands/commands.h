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
	bool build(std::string const& rootProjectName, bool verbose = false, bool noconsole=false, int jobs = 10);
	void clang();
	void clean();
	void cleanAll();
	void docu();
	void eclipse();
	void git(std::vector<std::string> const& _options);
	void test();
	void clone(std::string const& _url, std::string const& _dir);
	void pull();
	void push();
	void install();
	void status(std::string _buildMode = "");
	void quickFix();
	int  listFiles(std::string const& _relPath);
	void showDep(std::string const& rootProjectName);
	void toolchains();
	void toolchain(std::string const& _toolchain);


	auto getAllToolchains(aBuild::Workspace const& ws) -> std::map<std::string, aBuild::Toolchain>;
	auto getAllFlavors(aBuild::Workspace const& ws) -> std::map<std::string, aBuild::Flavor>;

	auto getAllInstallations(aBuild::Workspace const& ws) -> std::map<std::string, aBuild::Installation>;
	void checkingMissingPackages  (aBuild::Workspace& ws);
	void checkingNotNeededPackages(aBuild::Workspace& ws);
	void checkingInvalidPackages  (aBuild::Workspace& ws);
	void checkingRequiredPackages (aBuild::Workspace& ws);
}
