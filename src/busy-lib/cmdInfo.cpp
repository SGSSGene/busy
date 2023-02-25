#include "Arguments.h"
#include "Desc.h"
#include "Process.h"
#include "Toolchain.h"
#include "Workspace.h"
#include "utils.h"


namespace {
auto _ = cliModeInfo.run([]() {
    auto workspace = Workspace{*cliBuildPath};
    updateWorkspace(workspace);

    auto rootDir = workspace.busyFile;
    rootDir.remove_filename();

    // load busyFile
    auto desc = busy::desc::loadDesc(workspace.busyFile, rootDir, workspace.buildPath);

    auto printTS = [&](std::string const& tsName) {
        fmt::print("{}:\n", tsName);
        auto const& ts = workspace.allSets.at(tsName);
        fmt::print("  type: {}\n", ts.type);
        fmt::print("  language: {}\n", ts.language);
        fmt::print("  precompiled: {}\n", ts.precompiled);
        fmt::print("  installed: {}\n", ts.installed);
        auto path = ts.path / "src" / ts.name;
        path = path.lexically_normal();
        fmt::print("  path: {}\n", path);
        fmt::print("  deps:\n");
        for (auto d : ts.dependencies) {
            fmt::print("    - {}\n", d);
        }
    };
    for (auto ts : desc.translationSets) {
        workspace.allSets[ts.name] = ts;
        printTS(ts.name);
    }
    workspace.save();
    exit(0);
});

}
