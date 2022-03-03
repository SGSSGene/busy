#include "completion.h"

#include "utils.h"
#include "toolchains.h"

#include <filesystem>

namespace busy::comp {

auto toolchain(std::vector<std::string> const& /*str*/) -> std::pair<bool, std::set<std::string>> {
    auto ret = std::pair<bool, std::set<std::string>>{false, {}};
    auto workPath = std::filesystem::current_path();
    auto config = loadConfig(workPath, *cfgBuildPath, {cfgBusyPath, *cfgBusyPath});

    auto packages = std::vector<std::filesystem::path>{};
    if (!config.rootDir.empty() and config.rootDir != "." and std::filesystem::exists(config.rootDir)) {
        auto [pro, pack] = busy::readPackage(config.rootDir, config.busyFile);
        for (auto const& p : pack) {
            packages.emplace_back(p);
        }
    }

    packages.insert(begin(packages), user_sharedPath);
    packages.insert(begin(packages), global_sharedPath);
    for (auto const&  [name, path] : searchForToolchains(packages)) {
        ret.second.insert(name);
    }
    return ret;
}

auto options(std::vector<std::string> const& str) -> std::pair<bool, std::set<std::string>> {
    auto ret = std::pair<bool, std::set<std::string>>{false, {}};
    auto workPath = std::filesystem::current_path();
    auto config = loadConfig(workPath, *cfgBuildPath, {cfgBusyPath, *cfgBusyPath});

    auto toolchainOptions = getToolchainOptions(config.toolchain.name, config.toolchain.call);
    for (auto const& opt : toolchainOptions) {
        if (config.toolchain.options.count(opt.first) == 0) {
            ret.second.insert(opt.first);
        } else {
            ret.second.insert("no-" + opt.first);
        }
    }
    for (auto s : str) {
        ret.second.erase(s);
    }
    return ret;
}
auto projects(std::vector<std::string> const& str) -> std::pair<bool, std::set<std::string>> {
    auto ret = std::pair<bool, std::set<std::string>>{false, {}};
    auto workPath = std::filesystem::current_path();
    auto config = loadConfig(workPath, *cfgBuildPath, {cfgBusyPath, *cfgBusyPath});

    auto [projects, packages] = busy::readPackage(config.rootDir, config.busyFile);
    for (auto const& p : projects) {
        ret.second.insert(p.getName());
    }
    for (auto s : str) {
        ret.second.erase(s);
    }
    return ret;
}

auto sharedLibraries(std::vector<std::string> const& str) -> std::pair<bool, std::set<std::string>> {
    auto ret = std::pair<bool, std::set<std::string>>{false, {}};
    auto workPath = std::filesystem::current_path();
    auto config = loadConfig(workPath, *cfgBuildPath, {cfgBusyPath, *cfgBusyPath});

    auto [projects, packages] = busy::readPackage(config.rootDir, config.busyFile);
    auto projects_with_deps = createTranslationSets(projects);

    for (auto const& [project, deps] : projects_with_deps) {
        if (getTargetType(*project, deps, config.sharedLibraries) == TargetType::StaticLibrary) {
            ret.second.insert(project->getName());
        }
    }
    for (auto s : str) {
        ret.second.erase(s);
    }
    return ret;

}
auto staticLibraries(std::vector<std::string> const& str) -> std::pair<bool, std::set<std::string>> {
    auto ret = std::pair<bool, std::set<std::string>>{false, {}};
    auto workPath = std::filesystem::current_path();
    auto config = loadConfig(workPath, *cfgBuildPath, {cfgBusyPath, *cfgBusyPath});

    auto [projects, packages] = busy::readPackage(config.rootDir, config.busyFile);
    auto projects_with_deps = createTranslationSets(projects);

    for (auto const& [project, deps] : projects_with_deps) {
        if (getTargetType(*project, deps, config.sharedLibraries) == TargetType::SharedLibrary) {
            ret.second.insert(project->getName());
        }
    }
    for (auto s : str) {
        ret.second.erase(s);
    }
    return ret;
}




}
