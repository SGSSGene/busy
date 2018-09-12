#pragma once

#include "traits.h"
#include "Node.h"
#include "convert.h"
#include <iostream>

namespace fon {

template <typename Cb, typename T>
void visit(Cb cb, T& obj) {
	auto node = Node{cb, rootname{}, (Node<Cb, rootname, std::nullptr_t> const*)nullptr};
	node % obj;
}

template <typename F, typename Cb, typename T>
void filter(Cb cb, T& obj) {
	visit([&](auto& node, auto& obj) {
		using Node  = std::decay_t<decltype(node)>;
		using Value = std::decay_t<decltype(obj)>;

		if constexpr (Node::is_key) {
			return;
		}

		if constexpr(std::is_base_of_v<F, Value> or std::is_same_v<F, Value>) {
			cb(node, obj);
		} else if constexpr(std::is_polymorphic_v<F> and std::is_polymorphic_v<Value>) {
			auto* newObj = dynamic_cast<F*>(&obj);
			if (newObj) {
				cb(node, newObj);
			}
		} else if constexpr (Node::is_owner) {
			fon::convert(node, obj);
		}
	}, obj);
}

template <template <typename...> typename F, typename Cb, typename T>
void filter(Cb cb, T& obj) {
	visit([&](auto& node, auto& obj) {
		using Node = std::decay_t<decltype(node)>;
		if constexpr (is_same_base_v<F, std::decay_t<decltype(obj)>>) {
			cb(node, obj);
		} else if constexpr (Node::is_owner) {
			fon::convert(node, obj);
		}
	}, obj);
}

template <Type type, typename Cb, typename T>
void filter(Cb cb, T& obj) {
	visit([&](auto& node, auto& obj) {
		using Node = std::decay_t<decltype(node)>;
		if constexpr (Node::type == type) {
			cb(node, obj);
		} else if constexpr (Node::is_owner) {
			fon::convert(node, obj);
		}
	}, obj);
}

template <typename BaseType, typename T, typename L>
void findPath(T& input, BaseType* target, L l) {
	bool found{false};

	filter<BaseType>([&](auto& node, BaseType& obj) {
		using Node   = std::decay_t<decltype(node)>;
		if (found) {
			return;
		}

		if (&obj == target) {
			l(node);
			found = true;
			return;
		}

		if constexpr (Node::is_owner) {
			fon::convert(node, obj);
		}
	}, input);
}
template <typename BaseType, typename ...Args, typename T, typename L>
void findObj(T& input, std::string const& path, L l) {
	bool found{false};

	std::cout << "start comparing--\n";
	filter<BaseType>([&](auto& node, auto& obj) {
		std::cout << "compare: " << node->getPath() << " == " << path << "\n";

		using Node   = std::decay_t<decltype(node)>;
		if (found) {
			return;
		}

		std::cout << "compare: " << node->getPath() << " == " << path << "\n";
		if (node->getPath() == path) {
			l(obj);
			found = true;
		}

		if constexpr (Node::is_owner) {
			fon::convert(node, obj);
		}
	}, input);
}


}
