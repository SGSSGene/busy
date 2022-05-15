#include "CompilePipe.h"
#include "utils.h"

namespace busy {

auto CompilePipe::loadGraph() -> G {
    auto nodes = G::Nodes{};
    auto edges = G::Edges{};

    for (auto& [project, dep] : projects_with_deps) {
        nodes.push_back(project);
        for (auto& file : project->getFiles()) {
            nodes.emplace_back(&file);
            edges.emplace_back(G::Edge{&file, project});
        }
        for (auto& d : std::get<0>(dep)) {
            edges.emplace_back(G::Edge{d, project});
        }
    }
    return Graph{std::move(nodes), std::move(edges)};
}

auto CompilePipe::setupTranslationUnit(busy::File const& file) const -> std::vector<std::string> {
    auto outFile = file.getPath().lexically_normal().replace_extension(".o");
    auto inFile  = file.getRoot() / file.getPath();

    auto& project = graph.find_outgoing<busy::TranslationSet const>(&file);

    auto params = std::vector<std::string>{};

    params.emplace_back(toolchainCall);
    params.emplace_back("compile");
    params.emplace_back(project.getPath());
    params.emplace_back(inFile);
    params.emplace_back("obj" / outFile);
    // add all options
    params.emplace_back("--options");
    for (auto const& o : toolchainOptions) {
        params.emplace_back(o);
    }
    if (params.back() == "--options") params.pop_back();


    // add all include paths
    params.emplace_back("--ilocal");

    params.emplace_back(project.getPath());
    for (auto const& p : project.getLegacyIncludePaths()) {
        params.emplace_back(p);
    }
    if (params.back() == "--ilocal") params.pop_back();

    params.erase(std::unique(begin(params), end(params)), end(params));
    return params;
}


auto CompilePipe::setupTranslationSet(busy::TranslationSet const& project) const -> std::tuple<std::vector<std::string>, std::unordered_set<TranslationSet const*>> {

    auto [action, target] = [&]() -> std::tuple<std::string, std::filesystem::path> {
        auto type = getTargetType(project, projects_with_deps.at(&project), sharedLibraries);
        if (type == TargetType::Executable) {
            return {"executable", std::filesystem::path{"bin"} / project.getName()};
        } else if (type == TargetType::SharedLibrary) {
            return {"shared_library", (std::filesystem::path{"lib"} / project.getName()).replace_extension(".so")};
        }
        return {"static_library", (std::filesystem::path{"lib"} / project.getName()).replace_extension(".a")};
    }();

    auto params = std::vector<std::string>{};
    params.emplace_back(toolchainCall);
    params.emplace_back("link");
    params.emplace_back(action);

    params.emplace_back(target);

    params.emplace_back("--options");
    for (auto const& o : toolchainOptions) {
        params.emplace_back(o);
    }
    if (params.back() == "--options") params.pop_back();


    params.emplace_back("--input");
    for (auto file : graph.find_incoming<busy::File>(&project)) {
        if (colors.at(file) == Color::Compilable) {
            auto objPath = "obj" / file->getPath();
            objPath.replace_extension(".o");
            params.emplace_back(objPath);
        }
    }
    if (params.back() == "--input") params.pop_back();


    params.emplace_back("--llibraries");
    // add all legacy system libraries
    std::vector<std::string> systemLibraries;
    auto addSystemLibraries = [&](busy::TranslationSet const& project) {
        for (auto const& l : project.getSystemLibraries()) {
            auto iter = std::find(begin(systemLibraries), end(systemLibraries), l);
            if (iter != end(systemLibraries)) {
                systemLibraries.erase(iter);
            }
            systemLibraries.push_back(l);
        }
    };

    addSystemLibraries(project);

    auto dependencies = std::unordered_set<TranslationSet const*>{};
    graph.visit_incoming<TranslationSet const>(&project, [&](TranslationSet const& project) {
        if (colors.at(&project) == Color::Compilable) {
            auto target = [&]() {
                auto type = getTargetType(project, projects_with_deps.at(&project), sharedLibraries);
                if (type == TargetType::SharedLibrary) {
                    return (std::filesystem::path{"lib"} / project.getName()).replace_extension(".so");
                } else {
                    return (std::filesystem::path{"lib"} / project.getName()).replace_extension(".a");
                }
            }();
            params.emplace_back(target);
        }
        addSystemLibraries(project);
        dependencies.insert(&project);
    });
    if (params.back() == "--llibraries") params.pop_back();


    params.emplace_back("--syslibraries");
    for (auto const& l : systemLibraries) {
        params.emplace_back(l);
    }
    if (params.back() == "--syslibraries") params.pop_back();

    params.erase(std::unique(begin(params), end(params)), end(params));
    return std::tuple{params, dependencies};
}

}
