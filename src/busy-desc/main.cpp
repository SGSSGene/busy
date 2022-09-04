#include "Desc.h"
#include "genCall.h"
#include "answer.h"
#include "Process.h"
#include "Toolchain.h"

#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <queue>
#include <unordered_map>
#include <unordered_set>


using TranslationMap = std::unordered_map<std::string, busy::desc::TranslationSet>;
struct Workspace {
    std::filesystem::path  buildPath;
    std::filesystem::path  busyConfigFile;
    std::filesystem::path  busyFile;
    TranslationMap         allSets;
    std::vector<Toolchain> toolchains;


    Workspace(std::filesystem::path const& _buildPath)
        : buildPath{_buildPath}
    {}

    void loadOrInit() {
        std::error_code ec;
        create_directories(buildPath, ec);
        if (ec) {
            throw std::runtime_error("build folder " + buildPath.string() + " could not be created: "
                                     + ec.message());
        }
        busyConfigFile = buildPath / "busy_config.yaml";

        if (exists(busyConfigFile)) {
            auto node = YAML::LoadFile(busyConfigFile.string());
            auto config_version = node["config-version"].as<std::string>("");
            if (config_version == "1") {
                // load busyFile
                busyFile = node["busyFile"].as<std::string>();
                if (busyFile.is_relative()) {
                    busyFile = relative(buildPath / busyFile);
                }
                // load toolchains
                if (node["toolchains"].IsSequence()) {
                    for (auto e : node["toolchains"]) {
                        toolchains.emplace_back(buildPath, e.as<std::string>());
                        toolchains.back().detectLanguages();
                    }
                }
            } else {
                throw std::runtime_error("unknown config-version: " + config_version);
            }
        }
    }

    void save() {
        auto node = YAML::Node{};
        node["config-version"] = "1";
        if (busyFile.is_relative()) {
            node["busyFile"] = relative(busyFile, buildPath).string();
        } else {
            node["busyFile"] = busyFile.string();
        }
        for (auto const& t : toolchains) {
            node["toolchains"].push_back(t.toolchain.string());
        }
        auto ofs = std::ofstream{busyConfigFile};
        ofs << node;
    }

    /** Returns a list of TranslationSets that tu is depending on
     */
    auto findDependencies(busy::desc::TranslationSet const& tu) const {
        auto deps  = std::vector<busy::desc::TranslationSet>{};
        auto found = std::unordered_set<std::string>{};

        auto open = std::queue<std::string>{};
        for (auto d : tu.dependencies) {
            open.emplace(d);
        }
        found.insert(tu.name);

        while (!open.empty()) {
            auto n = open.front();
            open.pop();
            found.insert(n);
            deps.push_back(allSets.at(n));
            for (auto d : allSets.at(n).dependencies) {
                open.emplace(d);
            }
        }

        return deps;
    }

    /** returns tool change with appropiate language
     */
    auto getToolchain(std::string const& lang) const -> Toolchain const& {
        for (auto const& t : toolchains) {
            for (auto const& l : t.languages) {
                if (l == lang) {
                    return t;
                }
            }
        }
        throw std::runtime_error("No toolchain found which provides: " + lang);
    }

    /** Translates a tu and its dependencnies
     */
    void translate(busy::desc::TranslationSet const& ts) const {
        auto deps = findDependencies(ts);
        for (auto d : deps) {
            translate(d);
        }
        std::cout << "\n\nTRANSLATING: " << ts.name << "\n";
        if (ts.precompiled) {
            return;
        }

        auto& toolchain = getToolchain(ts.language);

        toolchain.setupTranslationSet(ts, deps);

        auto objFiles = std::vector<std::filesystem::path>{};
        auto tsPath = ts.path / "src" / ts.name;
        for (auto f : std::filesystem::recursive_directory_iterator(tsPath)) {
            if (f.is_regular_file()) {
                auto tuPath = relative(f, tsPath);
                objFiles.emplace_back(tuPath.string());
                auto [call, answer] = toolchain.translateUnit(ts, tuPath);
                std::cout << call << "\n";
                std::cout << answer.stdout << "\n";
                std::cout << "\n";
            }
        }
        toolchain.finishTranslationSet(ts, objFiles, deps);
    }
};

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
            workspace.loadOrInit();
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
