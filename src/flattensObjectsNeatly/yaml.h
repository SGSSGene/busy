#pragma once


#include <yaml-cpp/yaml.h>
#include <optional>
#include <tuple>
#include "utils.h"

namespace fon::yaml {

namespace details {

template <size_t I=1, typename ...Args>
auto accessNode(std::tuple<rootname, Args...> const& key, YAML::Node& node) {
	if constexpr (I <= sizeof...(Args)) {
		using Key = std::decay_t<decltype(std::get<I>(key))>;
		auto n = [&]() {
			if constexpr (std::is_same_v<Key, std::string_view>) {
				return node[std::string{std::get<I>(key)}];
			} else {
				return node[std::get<I>(key)];
			}
		}();
		return accessNode<I+1>(key, n);
	} else {
		return node;
	}
}

template <typename Key>
auto toKey(Key key) {
	if constexpr(std::is_same_v<Key, std::string> or std::is_arithmetic_v<Key>) {
		return key;
	} else if constexpr (not std::is_same_v<Key, rootname>) {
		return std::string{key};
	}
}

template <typename Stack, typename Node, typename ValueT>
void push_pop_key(Stack& stack, Node& node, ValueT& obj) {
	if constexpr (Node::push_key) {
		using Key = std::decay_t<decltype(node->getKey())>;
		if constexpr (not std::is_same_v<Key, rootname>) {
			stack.push_back(stack.back()[toKey(node->getKey())]);
		}
	} else if constexpr (Node::pop_key) {
		stack.pop_back();
	}
}


template <typename T>
auto serialize(T const& _input, YAML::Node root = {}) -> YAML::Node {
	auto& input = const_cast<T&>(_input);

	std::vector<YAML::Node> stack{root};

	fon::visit([&](auto& node, auto& obj) {
		using Node   = std::decay_t<decltype(node)>;
		using ValueT = std::decay_t<decltype(obj)>;
		if constexpr (Node::is_key) {
			push_pop_key(stack, node, obj);
		} else if constexpr (Node::pop_key) {
			stack.pop_back();
		} else if constexpr (std::is_enum_v<ValueT>) {
			stack.back() = static_cast<std::underlying_type_t<ValueT>>(obj);
		} else if constexpr (Node::is_value) {
			stack.back() = obj;
		} else if constexpr (Node::is_map or Node::is_list or Node::is_object) {
			Node::range(obj, [&](auto& key, auto& value) {
				node[key] % value;
			});
		} else if constexpr (Node::is_pointer) {
			if constexpr (is_same_base_v<std::unique_ptr, ValueT>) {
				node % *obj;
			} else {
				findPath(input, obj, [&](auto& node) {
					stack.back() = node->getPath();
				});
			}
		}
	}, input);

	return stack.back();
}

template <typename T>
auto deserialize(YAML::Node root) -> T {
	std::vector<YAML::Node> stack{root};

	auto res = getEmpty<T>();
	visit([&](auto& node, auto& obj) {
		using Node   = std::decay_t<decltype(node)>;
		using ValueT = std::decay_t<decltype(obj)>;

		if constexpr (Node::is_key) {
			push_pop_key(stack, node, obj);
		} else if constexpr (std::is_enum_v<ValueT>) {
			using UT = std::underlying_type_t<ValueT>;
			obj = ValueT{stack.back().template as<UT>()};
		} else if constexpr (Node::is_value) {
			obj = stack.back().template as<ValueT>();
		} else if constexpr (Node::is_list) {
			Node::reserve(obj, stack.back().size());
			for (size_t idx{0}; idx < stack.back().size(); ++idx) {
				Node::emplace(node, obj, idx);
			}
		} else if constexpr (Node::is_map) {
			using Key   = typename Node::Key;
			auto y_node = stack.back();
			Node::reserve(obj, y_node.size());
			for (auto iter{y_node.begin()}; iter != y_node.end(); ++iter) {
				auto key = iter->first.as<Key>();
				Node::emplace(node, obj, key);
			}
		} else if constexpr (Node::is_pointer) {
			using BaseType = typename Node::Value;
			if constexpr (is_same_base_v<std::unique_ptr, ValueT>) {
				if (not obj) {
					obj.reset(new BaseType{Node::getEmpty()});
				}
				node % *obj;
			}
		} else if constexpr (Node::is_object) {
			Node::range(obj, [&](auto& key, auto& value) {
				if (not stack.back()[toKey(key)].IsDefined()) {
					return;
				}
				node[key] % value;
			});
		}
	}, res);
	visit([&](auto& node, auto& obj) {
		using Node   = std::decay_t<decltype(node)>;
		using ValueT = std::decay_t<decltype(obj)>;

		if constexpr (Node::is_key) {
			push_pop_key(stack, node, obj);
		} else if constexpr (Node::is_pointer) {
			using BaseType = typename Node::Value;
			if constexpr (not is_same_base_v<std::unique_ptr, ValueT> and not is_same_base_v<std::shared_ptr, ValueT>) {
				if (not stack.back().IsDefined()) {
					return;
				}
				std::cout << "searching for " << node->getPath() << "\n";
				findObj<BaseType>(res, stack.back().as<std::string>(), [&](auto& _obj) {
					obj = &_obj;
				});
			}
		} else {
			fon::convert(node, obj);
		}
	}, res);

	return res;
}
}
using details::serialize;
using details::deserialize;

}
