#include "Arguments.h"
#include "Desc.h"
#include "Process.h"
#include "Toolchain.h"
#include "Workspace.h"
#include "WorkQueue.h"
#include "utils.h"

#include <cassert>
#include <condition_variable>
#include <fmt/format.h>
#include <fmt/std.h>
#include <fstream>
#include <functional>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>

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
        if (verbose) {
            fmt::print("ts: {} (local)\n", ts.name);
        }
        workspace.allSets[ts.name] = ts;
        if (ts.type == "toolchain") {
            auto path = absolute(std::filesystem::path{ts.name} / "toolchain.sh");
            toolchains[ts.name] = path;
        }
    }
    return toolchains;
}


void app_main() {
    auto otherSet = cliModeStatus or cliModeInfo or cliModeInstall;
    if (!cliModeCompile and otherSet) return;
    auto workspace = Workspace{*cliBuildPath};
    updateWorkspace(workspace);

    auto toolchains = loadAllBusyFiles(workspace, cliVerbose);

    updateWorkspaceToolchains(workspace, toolchains);


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
