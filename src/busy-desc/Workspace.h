#pragma once

#include "Toolchain.h"

#include <filesystem>
#include <fstream>
#include <queue>
#include <string>
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
    {
        loadOrInit();
    }

private:
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
                busyFile = convertToRelativeByCwd(node["busyFile"].as<std::string>());
                // load toolchains
                if (node["toolchains"].IsSequence()) {
                    for (auto e : node["toolchains"]) {
                        toolchains.emplace_back(buildPath, e.as<std::string>());
                    }
                }
            } else {
                throw std::runtime_error("unknown config-version: " + config_version);
            }
        }
    }

public:

    auto convertToRelativeByBuildPath(std::filesystem::path const& p) const -> std::filesystem::path {
        if (p.is_relative()) {
            return relative(p, buildPath);
        }
        return p;
    }
    auto convertToRelativeByCwd(std::filesystem::path const& p) const -> std::filesystem::path {
        if (p.is_relative()) {
            return relative(buildPath / p);
        }
        return p;
    }

    void save() {
        auto node = YAML::Node{};
        node["config-version"] = "1";
        node["busyFile"] = convertToRelativeByBuildPath(busyFile).string();
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
    void translate(std::string const& tsName) const {
        auto& ts = allSets.at(tsName);

        auto deps = findDependencies(ts);
        for (auto d : deps) {
            translate(d.name);
        }
        fmt::print("\n\nTRANSLATING: {}\n", ts.name);
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
                fmt::print("{}\n{}\n\n", call, answer.stdout);
            }
        }
        toolchain.finishTranslationSet(ts, objFiles, deps);
    }

    /** Find all translation sets with executables
     */
    auto findExecutables() const -> std::vector<std::string> {
        auto res = std::vector<std::string>{};
        for (auto [name, ts] : allSets) {
            if (ts.type != "executable") continue;
            res.emplace_back(ts.name);
        }
        return res;
    }
};


