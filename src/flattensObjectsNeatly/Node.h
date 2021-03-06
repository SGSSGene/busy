#pragma once

#include "convert.h"

#include <string>
#include <string_view>
#include <variant>
#include <sstream>
#include <functional>


namespace fon {

template <typename Parent, typename T>
struct NodeWrapper : convert<Parent, T>::Infos {
    Parent const& mParent;

public:
    NodeWrapper()                              = delete;
    NodeWrapper(NodeWrapper const&)            = delete;
    NodeWrapper(NodeWrapper&&)                 = default;
    NodeWrapper& operator=(NodeWrapper const&) = delete;
    NodeWrapper& operator=(NodeWrapper&&)      = default;
    ~NodeWrapper()                             = default;

    NodeWrapper(Parent const& _other)
        : mParent{_other}
    {}

    static constexpr Type type            {convert<Parent, T>::type};
    static constexpr bool is_none         { type == Type::None };
    static constexpr bool is_value        { type == Type::Value };
    static constexpr bool is_convert      { type == Type::Convertible };
    static constexpr bool is_dynamic_list { type == Type::DynamicList };
    static constexpr bool is_map          { type == Type::Map };
    static constexpr bool is_object       { type == Type::Object };
    static constexpr bool is_pointer      { type == Type::Pointer };
    static constexpr bool is_owner        { not is_pointer or is_same_base_v<std::unique_ptr, T> or is_same_base_v<std::shared_ptr, T> };


    static auto getEmpty() {
        using Value = typename convert<Parent, T>::Infos::Value;
        return ::fon::getEmpty<Value>();
    }

    template <typename Key>
    auto operator[](Key key) const {
        return mParent.operator[](key);
    }

    template<typename T2>
    auto operator%(T2& t) const {
        return mParent.operator%(t);
    }
    auto* operator->() const {
        return &mParent;
    }
};

/*template <typename Parent, typename Key>
struct KeyedNode {
    Parent const& parent;
    Key key;
    KeyedNode(Parent const& parent, Key key)
        : parent{parent}
        , key{key}
    {}

    template<typename T>
    auto operator% (T& t) const {
        auto wrapper = NodeWrapper<Node, std::remove_const_t<T>>{*this};
        return cb(wrapper, t);
    }

};*/


template <typename Cb>
struct Node {
protected:
    Cb const& cb;

public:
    Node()                       = delete;
    Node(Node const&)            = delete;
    Node(Node&&)                 = default;
    Node& operator=(Node const&) = delete;
    Node& operator=(Node&&)      = default;
    ~Node()                      = default;

    Node(Cb const& _cb)
        : cb      {_cb}
    {}



    template <typename TKey>
    auto operator[](TKey key) const {
        return Node{cb};
    }

    // we assume all c-str are static values
    auto operator[](char const* key) const {
        return this->operator[](std::string_view{key});
    }

    template<typename T>
    auto operator% (T& t) const {
        auto wrapper = NodeWrapper<Node, std::remove_const_t<T>>{*this};
        return cb(wrapper, t);
    }
};

}
