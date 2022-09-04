#include "Desc.h"
#include "genCall.h"
#include "answer.h"
#include "Process.h"
#include "Toolchain.h"
#include "Workspace.h"

#include <fmt/format.h>
#include <fmt/std.h>
#include <fstream>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>

struct Arguments {
    std::string mode;
    std::filesystem::path buildPath{"."};
    std::optional<std::filesystem::path> busyFile;
    std::vector<std::filesystem::path> addToolchains;

    Arguments(int argc, char const* const* argv) {
        auto busyFile  = std::filesystem::path{};
        auto toolchains = std::vector<Toolchain>{};
        mode = argv[1];
        for (int i{2}; i < argc; ++i) {
            if (argv[i] == std::string_view{"-f"} and i+1 < argc) {
                ++i;
                busyFile = argv[i];
            } else if (argv[i] == std::string_view{"-t"} and i+1 < argc) {
                ++i;
                addToolchains.emplace_back(argv[i]);
            } else {
                if (buildPath.empty()) {
                    buildPath = argv[i];
                } else {
                    throw std::runtime_error("unexpected parameter: " + std::string{argv[i]});
                }
            }
        }
    }
};


int main(int argc, char const* argv[]) {
    if (argc < 1) return 1;

    // ./build/bin/busy-desc compile build -f busy3.yaml -t toolchains.d/gcc12.1.sh
    auto args = Arguments{argc, argv};


    // this will add cli options to the workspace
    auto updateWorkspace = [&](auto& workspace) {
        // set new busy file if set by commandline
        if (args.busyFile) {
            workspace.busyFile = *args.busyFile;
        }

        // add more toolchains if set by commandline
        for (auto&& t : args.addToolchains) {
            workspace.toolchains.emplace_back(args.buildPath, std::move(t));
        }
    };

    try {
        if (args.mode == "compile") {

            auto workspace = Workspace{args.buildPath};
            updateWorkspace(workspace);

            // load busyFile
            std::string mainExe; //!TODO smarter way to decide this ;-)
            auto desc = busy::desc::loadDesc(workspace.busyFile, workspace.buildPath);
            for (auto ts : desc.translationSets) {
                if (mainExe.empty()) mainExe = ts.name;
                workspace.allSets[ts.name] = ts;
            }
            auto root = workspace.allSets.at(mainExe);

            // translate main executable
            workspace.translate(root);
            workspace.save();
        } else if (args.mode == "status" || args.mode.empty()) {
            auto workspace = Workspace{args.buildPath};
            updateWorkspace(workspace);

            // load busyFile
            auto desc = busy::desc::loadDesc(workspace.busyFile, workspace.buildPath);

            fmt::print("available ts:\n");
            for (auto type : {"executable", "library"}) {
                fmt::print("  {}:\n", type);
                for (auto ts : desc.translationSets) {
                    if (ts.type != type) continue;
                    fmt::print("    - {}{}\n", ts.name, ts.precompiled?" (precompiled)":"");
                }
            }
            workspace.save();
        } else if (args.mode == "info") {
            auto workspace = Workspace{args.buildPath};
            updateWorkspace(workspace);

            // load busyFile
            auto desc = busy::desc::loadDesc(workspace.busyFile, workspace.buildPath);

            auto printTS = [&](std::string const& tsName) {
                fmt::print("{}:\n", tsName);
                auto const& ts = workspace.allSets.at(tsName);
                fmt::print("  type: {}\n", ts.type);
                fmt::print("  language: {}\n", ts.language);
                fmt::print("  precompiled: {}\n", ts.precompiled);
                fmt::print("  path: {}\n", ts.path / "src" / ts.name);
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
        } else {
            throw std::runtime_error("unknown mode: " + args.mode);
        }
    } catch (std::exception const& e) {
        fmt::print("{}\n", e.what());
    } catch (char const* p) {
        fmt::print("{}\n", p);
    }
}
