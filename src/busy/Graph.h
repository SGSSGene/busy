#pragma once

#include <cassert>
#include <list>
#include <queue>
#include <tuple>
#include <variant>
#include <vector>

namespace busy {

template <typename ...Args>
class Graph {
public:
    using Node = std::variant<Args*...>;
    using Edge = std::tuple<Node, Node>;
    using Nodes = std::vector<Node>;
    using Edges = std::vector<Edge>;

    struct IntNode {
        Node value;
        std::vector<IntNode const*> inNode;
        std::vector<IntNode const*> outNode;
    };

public:
    Nodes nodes;
    Edges edges;

    std::list<IntNode> intNodes;

public:
    [[nodiscard]]
    auto findIntNode(Node v) -> auto& {
        for (auto& n : intNodes) {
            if (n.value == v) {
                return n;
            }
        }
        assert(false);
    }

    [[nodiscard]]
    auto findIntNode(Node v) const -> auto const& {
        for (auto& n : intNodes) {
            if (n.value == v) {
                return n;
            }
        }
        assert(false);
    }

public:
    Graph(Nodes _nodes, Edges _edges)
        : nodes{std::move(_nodes)}
        , edges{std::move(_edges)}
    {
        for (auto const& n : nodes) {
            intNodes.emplace_back(IntNode{n, {}, {}});
        }
        for (auto const& [src, dst] : edges) {
            auto& srcNode = findIntNode(src);
            auto& dstNode = findIntNode(dst);
            srcNode.outNode.push_back(&dstNode);
            dstNode.inNode.push_back(&srcNode);
        }
    }

    template <typename T>
    [[nodiscard]]
    auto find_outgoing(Node n) -> T& {
        auto& node = findIntNode(n);
        for (auto const& n : node.outNode) {
            if (std::holds_alternative<T*>(n->value)) {
                return *std::get<T*>(n->value);
            }
        }
        assert(false);
    }

    template <typename T>
    [[nodiscard]]
    auto find_outgoing(Node n) const -> T const& {
        auto& node = findIntNode(n);
        for (auto const& n : node.outNode) {
            if (std::holds_alternative<T*>(n->value)) {
                return *std::get<T*>(n->value);
            }
        }
        assert(false);
    }


    template <typename T=void, typename L>
    void visit_incoming(Node n, L const& l) const {
        auto& node = findIntNode(n);

        auto queue = std::queue<IntNode const*>{};

        for (auto n : node.inNode) {
            queue.push(n);
        }

        while (not queue.empty()) {
            auto front = queue.front();
            queue.pop();
            if constexpr (std::is_void_v<T>) {
                std::visit([&](auto& ptr) {
                    l(*ptr);
                }, front->value);
            } else {
                if (std::holds_alternative<T*>(front->value)) {
                    l(*std::get<T*>(front->value));
                }
            }
            for (auto n : front->inNode) {
                queue.push(n);
            }
        }
    }

    template <typename T>
    [[nodiscard]]
    auto find_incoming(Node n) const -> std::vector<T const*> {
        auto result = std::vector<T const*>{};
        auto& node = findIntNode(n);
        for (auto const& n : node.inNode) {
            if (std::holds_alternative<T const*>(n->value)) {
                result.push_back(std::get<T const*>(n->value));
            }
        }
        return result;
    }
};

}
