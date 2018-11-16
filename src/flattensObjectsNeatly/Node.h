#pragma once

#include "convert.h"

#include <string>
#include <string_view>
#include <variant>
#include <sstream>
#include <functional>


namespace fon {

template <size_t N, typename L, typename ...Args>
void for_tuple(std::tuple<Args...> const& tuple, L const& l) {
	if constexpr (N < sizeof...(Args)) {
		l(std::get<N>(tuple));
		for_tuple<N+1>(tuple, l);
	}
}

template <typename L, typename ...Args>
void for_tuple(std::tuple<Args...> const& tuple, L const& l) {
	for_tuple<0>(tuple, l);
}

template <typename Parent, typename T>
struct NodeWrapper : convert<Parent, T>::Infos {
	Parent const& mParent;
	NodeWrapper(Parent const& _other)
		: mParent{_other}
	{}

	static constexpr Type type {convert<Parent, T>::type};
	static constexpr bool is_none    { type == Type::None };
	static constexpr bool is_value   { type == Type::Value };
	static constexpr bool is_convert { type == Type::Convertible };
	static constexpr bool is_list    { type == Type::List };
	static constexpr bool is_map     { type == Type::Map };
	static constexpr bool is_object  { type == Type::Object };
	static constexpr bool is_pointer { type == Type::Pointer };
	static constexpr bool is_owner   { not is_pointer or is_same_base_v<std::unique_ptr, T> or is_same_base_v<std::shared_ptr, T> };


	static auto getEmpty() {
		using Value = typename convert<Parent, T>::Infos::Value;
		return ::fon::getEmpty<Value>();
	}

	template <typename Key>
	auto operator[](Key key) const {
		return mParent.operator[](key);
	}

	template<typename T2>
	void operator%(T2& t) const {
		mParent.operator%(t);
	}
	auto* operator->() const {
		return &mParent;
	}
};

enum class noparent : uint8_t {};

template <typename Cb, typename Parent, typename ...Key>
struct Node {
protected:
	Cb const& cb;
	std::tuple<Key...> mKey;
	static_assert(sizeof...(Key) == 0 or sizeof...(Key) == 1, "must provide one or zero keys");
	Parent const* mParent;

public:
	Node(Cb const& _cb, std::tuple<Key...> _key, Parent const* _parent)
		: cb      {_cb}
		, mKey   {std::move(_key)}
		, mParent {_parent}
	{}

	~Node() {}


	template <typename TKey>
	auto operator[](TKey key) const {
		static_assert(is_any_of_v<TKey, std::string, std::string_view> or std::is_arithmetic_v<TKey>);
		return Node<Cb, Node, TKey>{cb, std::make_tuple(key), this};
	}

	// we assume all c-str are static values
	auto operator[](char const* value) const {
		return this->operator[](std::string_view{value});
	}

	template<typename T>
	void operator% (T& t) const {
		auto wrapper = NodeWrapper<Node, T>{*this};
		cb(wrapper, t);
	}

	auto getKey() const {
		return mKey;
	}

	template <typename L>
	auto visit(L const& l) const {
		if constexpr (not std::is_same_v<std::nullptr_t, Parent>) {
			mParent->visit(l);
		}
		l(*this);
	}

	auto getFullKey() const {
		if constexpr (std::is_same_v<std::nullptr_t, Parent>) {
			return getKey();
		} else {
			std::stringstream name;
			for_tuple(getKey(), [&](auto key) {
				name << "/" << key;
			});
			return std::tuple_cat(mParent->getFullKey(), getKey());
		}
	}

	auto getPath() const {
		std::stringstream name;
		for_tuple(getFullKey(), [&](auto key) {
			name << "/" << key;
		});
		return name.str();
	}
};

}
