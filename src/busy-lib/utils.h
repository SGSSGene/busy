#pragma once

#include "Workspace.h"

#include <filesystem>
#include <map>
#include <string>

auto loadAllBusyFiles(Workspace& workspace, bool verbose) -> std::map<std::string, std::filesystem::path>;

// this will add cli options to the workspace
void updateWorkspace(Workspace& workspace);
void updateWorkspaceToolchains(Workspace& workspace, std::map<std::string, std::filesystem::path> const& toolchains);
