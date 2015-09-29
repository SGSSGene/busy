#pragma once

#include "Workspace.h"

#include <memory>
#include <stdio.h>
#include <future>
#include "BuildActionClang.h"

#include "utils.h"
#include "Process.h"
#include "git.h"
#include "estd.h"


namespace commands {
	void build(bool verbose = false);
	void test();
	void clone(std::string const& _url, std::string const& _dir);
	void pull();
	void push();
	void install();
	void status(std::string _flavor = "");
	void quickFix();
	int  listFiles();
	void toolchains();
	void toolchain(std::string const& _toolchain);


	std::map<std::string, aBuild::Toolchain> getAllToolchains(aBuild::Workspace const& ws);
	std::map<std::string, aBuild::Installation> getAllInstallations(aBuild::Workspace const& ws);
	void checkingMissingPackages  (aBuild::Workspace& ws);
	void checkingNotNeededPackages(aBuild::Workspace& ws);
	void checkingInvalidPackages  (aBuild::Workspace& ws);
	void checkingRequiredPackages (aBuild::Workspace& ws);
}
