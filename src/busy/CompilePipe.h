#pragma once

#include "Package.h"
#include "Queue.h"

#include <cstddef>
#include <map>
#include <unordered_set>

namespace busy {

// first tuple entry in going edges, second entry outgoing edges
using TranslationSetMap = std::map<TranslationSet const*, std::tuple<std::set<TranslationSet const*>, std::set<TranslationSet const*>>>;

struct CompilePipe {
    using G        = Graph<TranslationSet const, File const>;
    using Q        = Queue<G>;
    enum class Color { Ignored, Compilable };
    using ColorMap = std::map<G::Node, Color>;

    std::string       toolchainCall;
    TranslationSetMap const& projects_with_deps;
    std::set<std::string> const& toolchainOptions;
    std::set<std::string> const& sharedLibraries;
    G                 graph;
    Q                 queue;
    ColorMap          colors;
    std::mutex        mutex;

    CompilePipe(std::string _toolchainCall, TranslationSetMap const& _projects_with_deps, std::set<std::string> const& _toolchainOptions, std::set<std::string> const& _sharedLibraries)
        : toolchainCall      {move(_toolchainCall)}
        , projects_with_deps {_projects_with_deps}
        , toolchainOptions   {_toolchainOptions}
        , sharedLibraries    {_sharedLibraries}
        , graph {loadGraph()}
        , queue {graph}
    {}

    [[nodiscard]]
    auto loadGraph() -> G;

    [[nodiscard]]
    auto setupCompiling (busy::File const& file) const -> std::vector<std::string>;

    [[nodiscard]] auto setupLinking(busy::TranslationSet const& project) const -> std::tuple<std::vector<std::string>, std::unordered_set<TranslationSet const*>>;


    [[nodiscard]]
    auto empty() const -> bool {
        return queue.empty();
    }

    [[nodiscard]]
    auto size() const -> size_t {
        return queue.size();
    }


    [[nodiscard]]
    auto extract(busy::File const& file) const -> std::tuple<std::vector<std::string>, std::nullptr_t> {
        return {setupCompiling(file), nullptr};
    }

    [[nodiscard]]
    auto extract(busy::TranslationSet const& project) const -> std::tuple<std::vector<std::string>, std::unordered_set<TranslationSet const*>> {
        return setupLinking(project);
    }

    auto pop() {
        return queue.pop();
    }
    template <typename Work, typename CB>
    void dispatch(Work work, CB const& cb) {
        queue.dispatch(work, [&](auto& x) {
            auto g = std::unique_lock{mutex};
            auto [params, dependencies] = extract(x);
            g.unlock();
            auto color = cb(x, params, dependencies);
            g.lock();
            colors[&x] = color;
        });
    }
};

}
