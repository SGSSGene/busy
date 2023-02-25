#include "Arguments.h"
#include "Desc.h"
#include "Process.h"
#include "Toolchain.h"
#include "Workspace.h"

auto loadAllBusyFiles(Workspace& workspace, bool verbose) -> std::map<std::string, std::filesystem::path>;

namespace {
auto _ = cliModeStatus.run([]() {
    // this will add cli options to the workspace
    auto updateWorkspace = [&](auto& workspace) {
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

    auto updateWorkspaceToolchains = [&](auto& workspace, std::map<std::string, std::filesystem::path> const& toolchains) {
        // add more toolchains if set by commandline
        for (auto t : *cliToolchains) {
            if (toolchains.find(t) == toolchains.end()) {
                throw "unknown toolchain";
            }
            workspace.toolchains.emplace_back(*cliBuildPath, toolchains.at(t));
        }
    };
    auto workspace = Workspace{*cliBuildPath};
    updateWorkspace(workspace);

    // load busy files
    auto toolchains = loadAllBusyFiles(workspace, cliVerbose);

    updateWorkspaceToolchains(workspace, toolchains);

    fmt::print("available ts:\n");
    for (auto type : {"executable", "library"}) {
        fmt::print("  {}:\n", type);
        for (auto const& [key, ts] : workspace.allSets) {
            if (ts.type != type) continue;
            fmt::print("    - {}{}{}\n", ts.name, ts.precompiled?" (precompiled)":"", ts.installed?" (installed)":"");
        }
    }
    fmt::print("available toolchains:\n");
    for (auto [key, value] : toolchains) {
        fmt::print("    {}: {}\n", key, value);
    }
    workspace.save();
    exit(0);
});

}
