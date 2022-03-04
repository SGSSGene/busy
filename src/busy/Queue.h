#pragma once

#include "Graph.h"

#include <cassert>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <variant>

namespace busy {


template <typename Graph>
class Queue {
public:
    using IntNode = typename Graph::IntNode;
    Graph const& graph;
    std::unordered_map<IntNode const*, size_t> inNodeCt;
    std::queue<IntNode const*> work;
    std::mutex mutex;

public:
    Queue(Graph const& _graph)
        : graph{_graph}
    {
        // set active in node count to zero
        for (auto& n : graph.intNodes) {
            inNodeCt.try_emplace(&n, 0);
        }

        for (auto& n : graph.intNodes) {
            auto ct = inNodeCt.at(&n);
            if (n.inNode.size() == ct) {
                work.push(&n);
            }
        }
    }

    [[nodiscard]]
    auto empty() const -> bool{
        return work.empty();
    }

    [[nodiscard]]
    auto size() const -> std::size_t {
        return work.size();
    }

    auto pop() {
        auto g = std::lock_guard{mutex};
        auto front = work.front();
        work.pop();
        return front;
    }

    template <typename L>
    void dispatch(IntNode const* node, L const& l) {
        std::visit([&](auto ptr) {
            l(*ptr);
        }, node->value);

        auto g = std::lock_guard{mutex};
        for (auto n : node->outNode) {
            auto& ct = inNodeCt.at(n);
            ct += 1;
            if (n->inNode.size() == ct) {
                work.push(n);
            }
        }
    }
};

}
