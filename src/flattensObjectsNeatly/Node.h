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
//    static constexpr bool is_value        { type == Type::Value };
    static constexpr bool is_convert      { type == Type::Convertible };
    static constexpr bool is_list         { type == Type::List };
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

    template <typename Object>
    requires (is_convert or is_object or is_list or is_map)
    auto visit(Object& object) const {
        if constexpr (is_convert) {
            return this->template convert(*this, object);
        } else if constexpr (is_object) {
            this->template range(object, [&](auto& key, auto& value) {
                (*this)[key] % value;
            }, [&](auto& value) {
                *this % value;
            });
        } else if constexpr (is_list or is_map) {
            this->template range(object, [&](auto& key, auto& value) {
                (*this)[key] % value;
            });
        }
    }
    template <typename Object, typename CB>
    requires (is_list or is_map)
    void visit(Object& object, CB cb) const {
        this->template range(object, cb);
    }
    template <typename Object, typename CB1, typename CB2>
    requires (is_object)
    void visit(Object& object, CB1 cb1, CB2 cb2) const {
        this->template range(object, cb1, cb2);
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
