#pragma once

#include "Toolchain.h"

#include <filesystem>
#include <fmt/chrono.h>
#include <fmt/std.h>
#include <functional>
#include <fstream>
#include <mutex>
#include <queue>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>

using TranslationMap = std::unordered_map<std::string, busy::desc::TranslationSet>;
struct FileTimestampCache {

    std::map<std::filesystem::path, std::chrono::system_clock::time_point> cache;

    auto get(std::filesystem::path const& p) {
        auto iter = cache.find(p);
        if (iter == cache.end()) {
            cache.try_emplace(p, file_time(p));
            iter = cache.find(p);
        }
        return iter->second;
    }
};

struct Workspace {
    std::filesystem::path    buildPath;
    std::filesystem::path    busyConfigFile;
    std::filesystem::path    busyFile;
    TranslationMap           allSets;
    std::vector<Toolchain>   toolchains;
    std::vector<std::string> options{"debug"};

    FileTimestampCache     fileModTime;
    bool firstLoad{true};

    struct FileInfo {
        bool    noCompilation{};
        std::chrono::system_clock::time_point lastCompile{};
        double  duration{};
        std::vector<std::filesystem::path> dependencies;
    };

    std::map<std::filesystem::path, FileInfo> fileInfos;

    std::mutex              mutex;

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
            firstLoad = false;
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
                // load options
                options.clear();
                if (node["options"].IsSequence()) {
                    for (auto e : node["options"]) {
                        options.emplace_back(e.as<std::string>());
                    }
                }
                if (node["fileInfos"].IsSequence()) {
                    for (auto e : node["fileInfos"]) {
                        auto name          = e["name"].as<std::string>();
                        auto noCompilation = e["noCompilation"].as<bool>(false);
                        using namespace std::chrono;
                        auto lastCompile   = system_clock::time_point{system_clock::duration{e["lastCompile"].as<int64_t>()}};
                        auto duration      = e["duration"].as<double>();
                        auto deps          = std::vector<std::filesystem::path>{};
                        if (e["dependencies"].IsSequence()) {
                            for (auto n : e["dependencies"]) {
                                deps.push_back(n.as<std::string>());
                            }
                        }
                        fileInfos.try_emplace(name, FileInfo{noCompilation, lastCompile, duration, deps});
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
        for (auto const& o : options) {
            node["options"].push_back(o);
        }
        for (auto const& [path, value] : fileInfos) {
            auto n = YAML::Node{};
            n["name"]          = path.string();
            n["noCompilation"] = value.noCompilation;
            using namespace std::chrono;
            n["lastCompile"]   = value.lastCompile.time_since_epoch().count();
            n["duration"]      = value.duration;
            for (auto d : value.dependencies) {
                n["dependencies"].push_back(d.string());
            }
            node["fileInfos"].push_back(n);
        }

        auto ofs = std::ofstream{busyConfigFile};
        ofs << node;
    }

    /** Returns a list of TranslationSets that ts is depending on
     */
    auto findDependencies(busy::desc::TranslationSet const& ts) const {
        auto deps  = std::vector<busy::desc::TranslationSet>{};
        auto found = std::unordered_set<std::string>{};

        auto open = std::queue<std::string>{};
        for (auto d : ts.dependencies) {
            open.emplace(d);
        }
        found.insert(ts.name);
        while (!open.empty()) {
            auto n = open.front();
            open.pop();
            found.insert(n);
            if (allSets.find(n) == allSets.end()) {
               throw std::runtime_error("dependency \"" + n + "\" not found");
            }
            deps.push_back(allSets.at(n));
            for (auto d : allSets.at(n).dependencies) {

                open.emplace(d);
            }
        }

        return deps;
    }

    auto findDependencyNames(std::string const& tsName) const {
        auto ss = std::unordered_set<std::string>{};
        auto& ts = allSets.at(tsName);
        for (auto s : findDependencies(ts)) {
            ss.insert(s.name);
        }
        return ss;
    }

    auto findDependencyNames(std::span<std::string const> tsNames) const {
        auto ss = std::unordered_set<std::string>{};
        for (auto const& tsName : tsNames) {
            auto deps = findDependencyNames(tsName);
            ss.insert(deps.begin(), deps.end());
        }
        return ss;
    }


    /** returns tool change with appropriate language
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

    auto _translateSetup(std::string const& tsName, bool verbose) {
        auto const& ts = allSets.at(tsName);

        if (ts.installed) {
            return;
        }
        if (verbose) {
            fmt::print("\n\nBegin translation set: {}\n", tsName);
        }
        auto deps      = findDependencies(ts);
        auto toolchain = getToolchain(ts.language);

        toolchain.setupTranslationSet(ts, deps, verbose);
    }
    auto _listTranslateUnits(std::string const& tsName) const {
        auto const& ts = allSets.at(tsName);
        auto tsPath = ts.path / "src" / tsName;

        auto units = std::vector<std::string>{};
        if (ts.precompiled || ts.installed) {
            return units;
        }
        for (auto _f : std::filesystem::recursive_directory_iterator(tsPath)) {
            if (!_f.is_regular_file()) continue;
            auto f = std::filesystem::path{_f};
            auto name = tsName + "/" + relative(_f, tsPath).string();
            units.emplace_back(f.string());
        }
        return units;
    }
    /** Returns a message why recompilation is required
     * otherwise the optional object is std::nullopt
     */
    auto _translateUnitRequiresCompilation(std::string const& tsName, std::string const& unit, bool forceCompilation) -> std::optional<std::string> {
        auto const& ts = allSets.at(tsName);
        auto f         = std::filesystem::path{unit};
        auto tsPath    = ts.path / "src" / tsName;

        auto g         = std::unique_lock{mutex};
        auto tuPath    = relative(f, tsPath);
        auto& finfo    = fileInfos[tsName / tuPath];

        if (forceCompilation) return "forced";
        if (fileModTime.get(f) > finfo.lastCompile) {
            return fmt::format("modification time of file is newer than object file {} > {}", fileModTime.get(f), finfo.lastCompile);
        }
        try {
            for (auto d : finfo.dependencies) {
                if (fileModTime.get(buildPath / d) > finfo.lastCompile) {
                    return fmt::format("dependend file has changed ({})", d);
                }
            }
        } catch(std::exception const& e) {
            return fmt::format("new dependency discovered"); //!TODO or was removed?
        }
        return std::nullopt;
    }
    void _translateUnit(std::string const& tsName, std::string const& unit, bool verbose, bool forceCompilation) {
        auto const& ts = allSets.at(tsName);
        auto tsPath    = ts.path / "src" / tsName;
        auto tuPath    = relative(std::filesystem::path{unit}, tsPath);

        auto recompile = _translateUnitRequiresCompilation(tsName, unit, forceCompilation);
        if (!recompile) {
            if (verbose) {
                fmt::print("no change: {} {}\n", tsName, unit);
            }
            return;
        }
        fmt::print("changed: {} {} - {}\n", tsName, unit, *recompile);

        auto toolchain = getToolchain(ts.language);
        auto [call, answer] = toolchain.translateUnit(ts, tuPath, verbose, options);

        if (verbose) {
            fmt::print("{}\n{}\n\n", call, answer.stdout);
            fmt::print("duration: {}\n", answer.compileDuration);
        }
        if (!answer.success) {
            throw std::runtime_error(fmt::format("error compiling:\n{}\n", answer.stderr));
        }

        auto g            = std::unique_lock{mutex};
        auto& finfo       = fileInfos[tsName / tuPath];
        finfo.lastCompile = answer.compileStartTime;
        finfo.duration    = answer.compileDuration;
        finfo.dependencies.clear();
        for (auto d : answer.dependencies) {
            finfo.dependencies.push_back(d);
        }
    }

    auto _translateLinkageRequiresWork(std::string const& tsName, bool forceCompilation) -> std::optional<std::string> {
        auto const& ts = allSets.at(tsName);
        auto tsPath    = ts.path / "src" / tsName;

        auto g         = std::unique_lock{mutex};
        auto& finfo    = fileInfos[tsName];

        if (forceCompilation) return "forced";
        for (auto const& unit : _listTranslateUnits(tsName)) {
            auto f         = std::filesystem::path{unit};
            auto tuPath    = relative(f, tsPath);
            if (fileModTime.get(f) > finfo.lastCompile) {
                return fmt::format("modification time of file is newer than linkage result ({}) {} > {}", f, fileModTime.get(f), finfo.lastCompile);
            }
        }
        try {
            for (auto d : finfo.dependencies) {
                if (fileModTime.get(buildPath / d) > finfo.lastCompile) {
                    return fmt::format("dependend file has changed ({})", d);
                }
            }
        } catch(std::exception const& e) {
            return fmt::format("new dependency discovered"); //!TODO or was removed?
        }
        return std::nullopt;
    }

    auto _translateLinkage(std::string const& tsName, bool verbose, bool forceCompilation) {
        auto const& ts = allSets.at(tsName);
        auto tsPath    = ts.path / "src" / tsName;
        auto deps      = findDependencies(ts);
        auto toolchain = getToolchain(ts.language);
        if (ts.installed) {
            if (verbose) {
                fmt::print("installed, no linkage {}\n", tsName);
            }
            return;
        }

        auto recompile = _translateLinkageRequiresWork(tsName, forceCompilation);
        if (!recompile) {
            if (verbose) {
                fmt::print("no change {}\n", tsName);
            }
            return;
        }
        fmt::print("changed, linking: {} - {}\n", tsName, *recompile);

        auto objFiles = std::vector<std::filesystem::path>{};
        for (auto const& unit : _listTranslateUnits(tsName)) {
            auto f         = std::filesystem::path{unit};
            auto tuPath    = relative(f, tsPath);
            objFiles.emplace_back(tuPath.string());
        }
        auto [call, answer] = toolchain.finishTranslationSet(ts, objFiles, deps, verbose, options);
        if (!answer.success) {
            throw std::runtime_error(fmt::format("error linking:\n{}\n", answer.stderr));
        }
        if (verbose) {
            fmt::print("\n\nFinished translation set: {}\n", ts.name);
        }

        auto g            = std::unique_lock{mutex};
        auto& finfo       = fileInfos[tsName];
        finfo.lastCompile = answer.compileStartTime;
        finfo.duration    = answer.compileDuration;
        finfo.dependencies.clear();
        for (auto d : answer.dependencies) {
            finfo.dependencies.push_back(d);
        }
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


