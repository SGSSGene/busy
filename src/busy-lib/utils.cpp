#include "Arguments.h"
#include "utils.h"

auto loadAllBusyFiles(Workspace& workspace, bool verbose) -> std::map<std::string, std::filesystem::path> {
    auto toolchains = std::map<std::string, std::filesystem::path>{};
    auto rootDir = workspace.busyFile;
    rootDir.remove_filename();
    // load other description files
    if (auto ptr = std::getenv("HOME")) {
        auto s = std::filesystem::path{ptr} / ".config/busy/env/share/busy";
        if (exists(s)) {
            for (auto const& d : std::filesystem::directory_iterator{s}) {
                if (!d.is_regular_file()) continue;
                auto desc = busy::desc::loadDesc(d.path(), rootDir, workspace.buildPath);
                for (auto ts : desc.translationSets) {
                    if (verbose) {
                        fmt::print("ts: {} (~/.config/busy/env/share/busy)\n", ts.name);
                    }
                    workspace.allSets[ts.name] = ts;
                    if (ts.type == "toolchain") {
                        auto path = absolute(d.path().parent_path().parent_path() / std::filesystem::path{ts.name} / "toolchain.sh");
                        toolchains[ts.name] = path;
                    }
                }
            }
        }
    }


    // load description as if "BUSY_ROOT" is the root, if non given, assuming "/usr"
    auto busy_root = [&]() -> std::filesystem::path {
        if (auto ptr = std::getenv("BUSY_ROOT")) {
            auto s = std::string{ptr};
            return s;
        }
        return "/usr";
    }();

    if (exists(busy_root / "share/busy")) {
        for (auto const& d : std::filesystem::directory_iterator{busy_root / "share/busy"}) {
            if (!d.is_regular_file()) continue;
            auto desc = busy::desc::loadDesc(d.path(), rootDir, workspace.buildPath);
            for (auto ts : desc.translationSets) {
                if (verbose) {
                    fmt::print("ts: {} (BUSY_ROOT)\n", ts.name);
                }
                workspace.allSets[ts.name] = ts;
                if (ts.type == "toolchain") {
                    auto path = absolute(d.path().parent_path().parent_path() / std::filesystem::path{ts.name} / "toolchain.sh");
                    toolchains[ts.name] = path;
                }
            }
        }
    }

    // load busyFile
    auto desc = busy::desc::loadDesc(workspace.busyFile, rootDir, workspace.buildPath);
    for (auto ts : desc.translationSets) {
        workspace.allSets[ts.name] = ts;
        if (ts.type == "toolchain") {
            auto path = absolute(std::filesystem::path{ts.name} / "toolchain.sh");
            toolchains[ts.name] = path;
        }
    }
    return toolchains;
}

// this will add cli options to the workspace
void updateWorkspace(Workspace& workspace) {
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

void updateWorkspaceToolchains(Workspace& workspace, std::map<std::string, std::filesystem::path> const& toolchains, std::vector<std::string> const& newToolchains) {
    // add more toolchains if set by commandline
    for (auto t : newToolchains) {
        if (toolchains.find(t) == toolchains.end()) {
            throw "unknown toolchain";
        }
        workspace.toolchains.emplace_back(*cliBuildPath, toolchains.at(t));
    }
};


void updateWorkspaceToolchains(Workspace& workspace, std::map<std::string, std::filesystem::path> const& toolchains) {
    updateWorkspaceToolchains(workspace, toolchains, *cliToolchains);
};

