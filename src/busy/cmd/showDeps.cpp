#include "../FileCache.h"
#include "../cache.h"
#include "../config.h"
#include "../utils.h"

namespace busy::cmd {
namespace {

void showDeps();

auto cmd     = sargp::Command{"show-deps", "show dependencies of projects", showDeps};
auto cfgTree = cmd.Flag("tree", "prints projects as a tree");


auto loadAllPackages(std::filesystem::path const& rootDir, std::filesystem::path const& busyFile) -> busy::Package {
    auto [translationSets, packages] = busy::readPackage(rootDir, busyFile);

    {
        auto packagesPath = std::filesystem::path{"packages"};
        if (is_directory(packagesPath)) {
            for (auto const& entry : std::filesystem::directory_iterator{packagesPath}) {
                packagesPath = relative(entry.path(), rootDir);
                auto [externalTranslationSets, packagePaths] = busy::readPackage(rootDir, packagesPath / "busy.yaml");
                for (auto& e : externalTranslationSets) {
                    translationSets.emplace_back(std::move(e));
                }
                for (auto& e : packagePaths) {
                    packages.emplace_back(std::move(e));
                }
            }
        }
    }
    return {translationSets, packages};
}



void showDeps() {
    auto workPath = std::filesystem::current_path();
    auto config   = loadConfig(workPath, *cfgBuildPath, {cfgBusyPath, *cfgBusyPath});

    auto [allProjects, packages] = loadAllPackages(config.rootDir, config.busyFile);

    packages.insert(begin(packages), user_sharedPath);
    packages.insert(begin(packages), global_sharedPath);

    // check consistency of packages
    fmt::print("checking consistency...");
    checkConsistency(allProjects);
    fmt::print("done\n");
    auto projects = normalizeTranslationSets(allProjects);


    auto projects_with_deps = createTranslationSets(projects);

    if (not *cfgTree) {
        printTranslationSets(projects_with_deps);
    } else {
        printTranslationSetTree(projects_with_deps);
    }
}

}
}
