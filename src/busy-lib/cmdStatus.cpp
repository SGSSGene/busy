#include "Arguments.h"
#include "Desc.h"
#include "Process.h"
#include "Toolchain.h"
#include "Workspace.h"
#include "utils.h"


namespace {
auto _ = cliModeStatus.run([]() {
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
