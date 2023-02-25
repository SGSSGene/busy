#pragma once

auto loadAllBusyFiles(Workspace& workspace, bool verbose) -> std::map<std::string, std::filesystem::path>;

// this will add cli options to the workspace
inline auto updateWorkspace(auto& workspace) {
    if (cliFile) {
        workspace.busyFile = *cliFile;
        return;
    }

    if (!workspace.firstLoad) return;

    for (auto p : {"busy.yaml", "../busy.yaml"}) {
        if (exists(std::filesystem::path{p})) {
            workspace.busyFile = p;
            return;
        }
    }
};

inline auto updateWorkspaceToolchains(auto& workspace, std::map<std::string, std::filesystem::path> const& toolchains) {
    // add more toolchains if set by commandline
    for (auto t : *cliToolchains) {
        if (toolchains.find(t) == toolchains.end()) {
            throw "unknown toolchain";
        }
        workspace.toolchains.emplace_back(*cliBuildPath, toolchains.at(t));
    }
};

