#pragma once

#include "traits.h"

#include <array>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>
#include <memory>

namespace fon {

enum class Type {
	None,
	Value,
	List,
	Map,
	Object,
	Pointer,
};

enum class index : size_t {};

// converter functions
template <typename Node, typename T, class Enable=void>
struct convert;

// deduction guide
template<typename Node, typename T> convert(Node&, T&) -> convert<Node, T>;

// special "none" types
template <typename Node>
struct convert<Node, std::string_view> {
	static constexpr Type type = Type::None;
	struct Infos {};

	convert(Node&, std::string_view const&) {}
};


// value types int, string, double, etc
template <typename Node>
struct convert<Node, std::string> {
	static constexpr Type type = Type::Value;
	struct Infos {};

	convert(Node&, std::string&) {}
};

template <typename Node, typename T>
struct convert<Node, T, typename std::enable_if_t<std::is_arithmetic_v<T>>> {
	static constexpr Type type = Type::Value;
	struct Infos {};

	convert(Node&, T&) {}
};

template <typename Node, typename T>
struct convert<Node, T, typename std::enable_if_t<std::is_enum_v<T>>> {
	static constexpr Type type = Type::Value;
	struct Infos {};

	convert(Node&, T&) {}
};

template <typename Iter>
struct Range {
	std::tuple<Iter, Iter> range;
	Range(Iter begin, Iter end)
		: range{begin, end}
	{}
	auto begin() const {
		return std::get<0>(range);
	}

	auto end() const {
		return std::get<1>(range);
	}
};

// list types
template <template <typename...> typename C, typename T>
constexpr static bool is_sequence_v = is_any_of_v<C<T>,
	std::vector<T>, std::list<T>, std::deque<T>, std::forward_list<T>>;

template <typename Node, typename T, template <typename...> typename C>
struct convert<Node, C<T>, typename std::enable_if_t<is_sequence_v<C, T>>> {
	static constexpr Type type = Type::List;
	struct Infos {
		using Key   = size_t;
		using Value = T;
		static auto range(C<T>& obj) {
			return Range{obj.begin(), obj.end()};
			
		};
		template <typename L>
		static void range(C<T>& obj, L l) {
			std::size_t i{0};
			for (auto& e : obj) {
				l(i, e);
				++i;
			}
		};

		static void reserve(C<T>& obj, size_t size) {
			obj.reserve(size);
		}
		template <typename N2>
		static auto emplace(N2& node, C<T>& obj, Key key) {
			obj.push_back(N2::getEmpty());
			node[key] % obj.back();
		}
	};

	convert(Node& node, C<T>& obj) {
		for (size_t i{0}; i < obj.size(); ++i) {
			node[i] % obj.at(i);
		}
	}
};

// map types
template <template <typename...> typename C, typename Key, typename T>
constexpr static bool is_map_v = is_any_of_v<C<Key, T>,
	std::map<Key, T>, std::unordered_map<Key, T>, std::multimap<Key, T>, std::unordered_multimap<Key, T>>;

template <typename Node, typename TKey, typename T, template <typename...> typename C>
struct convert<Node, C<TKey, T>, typename std::enable_if_t<is_map_v<C, TKey, T>>> {
	static constexpr Type type = Type::Map;
	struct Infos {
		using Key   = TKey;
		using Value = T;
		template <typename L>
		static void range(C<TKey, T>& obj, L l) {
			for (auto& [key, value] : obj) {
				l(key, value);
			}
		};

		static void reserve(C<TKey, T>& obj, size_t size) {
			(void)obj;
			(void)size;
		}
		template <typename N2>
		static auto emplace(N2& node, C<TKey, T>& obj, TKey key) {
			Value value = N2::getEmpty();
			node[key] % value;
			obj.emplace(std::move(key), std::move(value));
		}
	};
	convert(Node& node, C<TKey, T>& obj) {
		for (auto& [key, value] : obj) {
			node[key] % value;
		}
	}
};

namespace helper {
template <typename Cb, typename Key>
struct SubVisitor {

	Cb const& cb;
	Key key;
	SubVisitor(Cb const& _cb, Key _key)
		: cb{_cb}
		, key{_key}
	{}

	template <typename T>
	auto operator%(T& obj) {
		cb(key, obj);
	}
};
template <typename Cb>
struct Visitor {
	Cb const& cb;

	Visitor(Cb const& _cb)
		: cb{_cb}
	{};

	auto operator[](std::string_view key) {
		return SubVisitor{cb, key};
	}
};
}

// object types
template <typename Node, typename T>
struct convert<Node, T, typename std::enable_if_t<has_ser_v<Node, T>>> {
	static constexpr Type type = Type::Object;
	struct Infos {
		template <typename L>
		static auto range(T& obj, L l) {
			auto visitor = helper::Visitor{[l](auto& key, auto& obj) {
				l(key, obj);
			}};
			obj.serialize(visitor);
		};

	};

	convert(Node& node, T& obj) {
		obj.serialize(node);
	}
};


// pointer types
template <typename Node, typename T>
struct convert<Node, T*> {
	static constexpr Type type = Type::Pointer;
	struct Infos {
		using Value = T;
	};
	convert(Node& node, T*& obj) {
		if (obj) {
			node % *obj;
		}
	}
};

template <typename Node, typename T>
struct convert<Node, std::unique_ptr<T>> {
	static constexpr Type type = Type::Pointer;
	struct Infos {
		using Value = T;
	};
	convert(Node& node, std::unique_ptr<T>& obj) {
		if (obj) {
			node % *obj;
		}
	}
};


}
