#pragma once

#include <cassert>
#include <list>
#include <mutex>
#include <queue>
#include <tuple>
#include <variant>
#include <vector>

template <typename ...Args>
class Queue {
public:
	using Node = std::variant<Args*...>;
	using Edge = std::tuple<Node, Node>;
	using Nodes = std::vector<Node>;
	using Edges = std::vector<Edge>;

private:

	struct IntNode {
		Node value;
		int inNodeCt{0};
		std::vector<IntNode*> inNode;
		std::vector<IntNode*> outNode;
	};

	std::list<IntNode> nodes;
	std::queue<IntNode*> work;
	std::mutex mutex;

	auto& findNode(Node v) {
		for (auto& n : nodes) {
			if (n.value == v) {
				return n;
			}
		}
		assert(false);
	}
	auto const& findNode(Node v) const {
		for (auto& n : nodes) {
			if (n.value == v) {
				return n;
			}
		}
		assert(false);
	}

public:
	Queue(Nodes const& _nodes, Edges const& _edges) {
		for (auto n : _nodes) {
			nodes.emplace_back(IntNode{n, {}, {}});
		}
		for (auto [src, dst] : _edges) {
			auto& srcNode = findNode(src);
			auto& dstNode = findNode(dst);
			srcNode.outNode.push_back(&dstNode);
			dstNode.inNode.push_back(&srcNode);
		}

		for (auto& n : nodes) {
			if (n.inNode.size() == n.inNodeCt) {
				work.push(&n);
			}
		}
	}

	bool empty() const {
		return work.empty();
	}
	size_t size() const {
		return work.size();
	}

	template <typename T>
	auto find_outgoing(Node n) -> T& {
		auto& node = findNode(n);
		for (auto const& n : node.outNode) {
			if (std::holds_alternative<T*>(n->value)) {
				return *std::get<T*>(n->value);
			}
		}
		assert(false);
	}

	template <typename T>
	auto find_outgoing(Node n) const -> T const& {
		auto& node = findNode(n);
		for (auto const& n : node.outNode) {
			if (std::holds_alternative<T*>(n->value)) {
				return *std::get<T*>(n->value);
			}
		}
		assert(false);
	}


	template <typename T=void, typename L>
	void visit_incoming(Node n, L const& l) const {
		auto& node = findNode(n);

		std::queue<IntNode*> work;

		for (auto n : node.inNode) {
			work.push(n);
		}

		while (not work.empty()) {
			auto front = work.front();
			work.pop();
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
				work.push(n);
			}
		}
	}

	template <typename T>
	auto find_incoming(Node n) const -> std::vector<T const*> {
		std::vector<T const*> result;
		auto& node = findNode(n);
		for (auto const& n : node.inNode) {
			if (std::holds_alternative<T const*>(n->value)) {
				result.push_back(std::get<T const*>(n->value));
			}
		}
		return result;
	}

	auto pop() {
		auto g = std::lock_guard{mutex};
		auto front = work.front();
		work.pop();
		return front;
	}

	template <typename L>
	void dispatch(IntNode* node, L const& l) {
		std::visit([&](auto ptr) {
			l(*ptr);
		}, node->value);

		auto g = std::lock_guard{mutex};
		for (auto n : node->outNode) {
			n->inNodeCt += 1;
			if (n->inNode.size() == n->inNodeCt) {
				work.push(n);
			}
		}
	}
};
