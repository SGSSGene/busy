#include "Arguments.h"
#include "Desc.h"
#include "Process.h"
#include "Toolchain.h"
#include "Workspace.h"
#include "WorkQueue.h"
#include "utils.h"

#include <fmt/format.h>
#include <unordered_set>

void app_main() {
    auto otherSet = cliModeStatus or cliModeInfo or cliModeInstall;
    if (!cliModeCompile and otherSet) return;
    auto workspace = Workspace{*cliBuildPath};
    updateWorkspace(workspace);

    auto toolchains = loadAllBusyFiles(workspace, cliVerbose);

    updateWorkspaceToolchains(workspace, toolchains, *cliToolchains);


    auto root = [&]() -> std::vector<std::string> {
        //!TODO how to do trailing values in clice?
        //if (args.trailing.size()) {
        //    return {args.trailing.front()};
        //}
        return workspace.findExecutables();
    }();

    auto wq = WorkQueue{};
    auto all = workspace.findDependencyNames(root); // All Translation units which root depence on
    for (auto r : root) {
        all.insert(r);
    }
    for (auto ts : all) {
        wq.insert(ts + "/setup", [ts, &workspace, &wq]() {
            workspace._translateSetup(ts, cliVerbose);
        }, {});
        auto units = std::unordered_set<std::string>{};
        for (auto const& unit : workspace._listTranslateUnits(ts)) {
            wq.insert(ts + "/unit/" + unit, [ts, &workspace, unit]() {
                workspace._translateUnit(ts, unit, cliVerbose, cliClean, *cliOptions);
            }, {ts + "/setup"});
            units.emplace(ts + "/unit/" + unit);
        }
        units.emplace(ts + "/setup");
        for (auto dep : workspace.findDependencyNames(ts)) {
            units.emplace(dep + "/linkage");
        }
        wq.insert(ts + "/linkage", [ts, &workspace]() {
            workspace._translateLinkage(ts, cliVerbose, *cliOptions);
        }, units);
    }

    // translate all jobs
    std::atomic_bool errorAppeared{false};

    auto t = std::vector<std::jthread>{};
    for (ssize_t i{0}; i < *cliJobs; ++i) {
        t.emplace_back([&]() {
            try {
                while (!errorAppeared and wq.processJob());
            } catch(std::exception const& e) {
                if (!errorAppeared) {
                    fmt::print("compile error: {}\n", e.what());
                }
                errorAppeared = true;
                wq.flush();
            }
        });
    }
    t.clear();
    workspace.save();
    if (errorAppeared) {
        exit(1);
    }
}
