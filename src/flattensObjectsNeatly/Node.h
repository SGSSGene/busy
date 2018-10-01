#pragma once

#include "convert.h"

#include <string>
#include <string_view>
#include <variant>
#include <sstream>
#include <functional>

namespace fon {

enum class rootname : uint8_t {};

template <typename Parent, typename T, bool pushKey, bool popKey>
struct NodeWrapper : convert<Parent, T>::Infos {
	Parent const& mParent;
	NodeWrapper(Parent const& _other)
		: mParent{_other}
	{}

	static constexpr bool push_key   = pushKey;
	static constexpr bool pop_key    = popKey;

	static constexpr Type type = convert<Parent, T>::type;
	static constexpr bool is_value   = Type::Value   == type;
	static constexpr bool is_list    = Type::List    == type;
	static constexpr bool is_map     = Type::Map     == type;
	static constexpr bool is_object  = Type::Object  == type;
	static constexpr bool is_pointer = Type::Pointer == type;
	static constexpr bool is_owner   = not is_pointer or is_same_base_v<std::unique_ptr, T> or is_same_base_v<std::shared_ptr, T>;

	static constexpr bool is_key     = push_key or pop_key;


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

template <typename Cb, typename Key, typename Parent>
struct Node {
protected:
	Cb const& cb;
	Key mKey;
	Parent const* mParent;

public:
	Node(Cb const& _cb, Key _key, Parent const* _parent)
		: cb      {_cb}
		, mKey    {std::move(_key)}
		, mParent {_parent}
	{
		if constexpr (not std::is_same_v<Key, rootname>) {
			bool null{};
			auto wrapper = NodeWrapper<Node, bool, true, false>{*this};
			cb(wrapper, null);
		}
	}

	~Node() {
		if constexpr (not std::is_same_v<Key, rootname>) {
			bool null;
			auto wrapper = NodeWrapper<Node, bool, false, true>{*this};
			cb(wrapper, null);
		}
	}


	template <typename TKey>
	auto operator[](TKey key) const {
		static_assert(is_any_of_v<TKey, std::string, std::string_view> or std::is_arithmetic_v<TKey>);
		return Node<Cb, TKey, Node>{cb, key, this};
	}	

	// we assume all c-str are static values
	auto operator[](char const* value) const {
		return this->operator[](std::string_view{value});
	}

	template<typename T>
	void operator% (T& t) const {
		auto wrapper = NodeWrapper<Node, T, false, false>{*this};
		cb(wrapper, t);
	}

	auto getKey() const {
		return mKey;
	}

	template <typename L>
	auto visit(L l) const {
		if constexpr (not std::is_same_v<std::nullptr_t, Parent>) {
			mParent->visit(l);
		}
		l(*this);
	}

	auto getFullKey() const {
		if constexpr (std::is_same_v<Key, rootname>) {
			return std::tuple{getKey()};
		} else {
			return std::tuple_cat(mParent->getFullKey(), std::tuple{getKey()});
		}
	}

	auto getPath() const {
		std::stringstream name;
		visit([&](auto& node) {
			if constexpr (not std::is_same_v<std::decay_t<decltype(node.getKey())>, rootname>) {
				name << "/" << node.getKey();
			}
		});
		return name.str();
	}
};

}
