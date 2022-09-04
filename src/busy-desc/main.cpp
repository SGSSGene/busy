#include "Desc.h"
#include "genCall.h"
#include "answer.h"
#include "Process.h"
#include "Toolchain.h"
#include "Workspace.h"

#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>

struct Arguments {
    std::filesystem::path buildPath{"."};
    std::optional<std::filesystem::path> busyFile;
    std::vector<std::filesystem::path> addToolchains;

    Arguments(int argc, char const* const* argv) {
        auto busyFile  = std::filesystem::path{};
        auto toolchains = std::vector<Toolchain>{};
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
    if (argc < 2) return 1;

    // ./build/bin/busy-desc compile build -f busy3.yaml -t toolchains.d/gcc12.1.sh
    auto mode = std::string{argv[1]};

    try {
        if (mode == "compile") {
            auto args = Arguments{argc, argv};

            auto workspace = Workspace{args.buildPath};
            if (args.busyFile) {
                workspace.busyFile = *args.busyFile;
            }
            for (auto&& t : args.addToolchains) {
                workspace.toolchains.emplace_back(args.buildPath, std::move(t));
            }

            std::string mainExe; //!TODO smarter way to decide this ;-)
            auto desc = busy::desc::loadDesc(workspace.busyFile, workspace.buildPath);
            for (auto ts : desc.translationSets) {
                if (mainExe.empty()) mainExe = ts.name;
                workspace.allSets[ts.name] = ts;
                auto path = desc.path / "src" / ts.name;
            }
            auto root = workspace.allSets.at(mainExe);
            workspace.translate(root);
            workspace.save();
        }
    } catch (std::exception const& e) {
        fmt::print("{}\n", e.what());
    } catch (char const* p) {
        fmt::print("{}\n", p);
    }
}
