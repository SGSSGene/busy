#pragma once

#include "Workspace.h"

#include <memory>
#include <stdio.h>
#include <future>
#include <process/Process.h>
#include <busyUtils/busyUtils.h>

#include "git.h"
#include "estd.h"

namespace commands {
	using namespace busy;
	auto getAllToolchains(Workspace const& ws) -> std::map<std::string, Toolchain>;
	auto getAllFlavors(Workspace const& ws) -> std::map<std::string, Flavor>;

	auto getAllInstallations(Workspace const& ws) -> std::map<std::string, Installation>;
	void cloneMissingPackages  (Workspace const& ws);
	void checkingNotNeededPackages(Workspace& ws);
	void checkingInvalidPackages  (Workspace& ws);
	void checkingRequiredPackages (Workspace& ws);
}

