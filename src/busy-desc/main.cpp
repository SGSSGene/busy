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


int main(int argc, char const* argv[]) {
    if (argc < 2) return 1;

    // ./build/bin/busy-desc compile busy3.yaml build toolchains.d/gcc12.1.sh
    auto mode = std::string{argv[1]};

    try {
        if (mode == "compile") {
            auto buildPath = std::filesystem::path{};

            auto busyFile  = std::filesystem::path{};
            auto toolchains = std::vector<Toolchain>{};
            for (int i{2}; i < argc; ++i) {
                if (argv[i] == std::string_view{"-f"} and i+1 < argc) {
                    ++i;
                    busyFile = argv[i];
                } else if (argv[i] == std::string_view{"-t"} and i+1 < argc) {
                    ++i;
                    auto t = std::filesystem::path{argv[i]};
                    if (t.is_relative()) {
                        t = relative(absolute(t), buildPath);
                    }

                    toolchains.emplace_back(buildPath, std::move(t));
                    toolchains.back().detectLanguages();
                } else {
                    if (buildPath.empty()) {
                        buildPath = argv[i];
                    } else {
                        throw std::runtime_error("unexpected parameter: " + std::string{argv[i]});
                    }
                }
            }
            if (buildPath.empty()) {
                buildPath = ".";
            }


            auto workspace = Workspace{buildPath};
            if (!busyFile.empty()) {
                workspace.busyFile = busyFile;
            }
            for (auto&& t : toolchains) {
                workspace.toolchains.emplace_back(std::move(t));
            }
            toolchains.clear();

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
        std::cout << e.what() << "\n";
    } catch (char const* p) {
        std::cout << p << "\n";
    }
}
