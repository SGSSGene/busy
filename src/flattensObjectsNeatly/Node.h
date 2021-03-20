#pragma once

#include "convert.h"

#include <string>
#include <string_view>
#include <variant>
#include <sstream>
#include <functional>


namespace fon {

template <typename Parent, typename T>
struct NodeWrapper : convert<Parent, T> {
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
    static constexpr bool is_convert      { type == Type::Convertible };
    static constexpr bool is_list         { type == Type::List };
    static constexpr bool is_map          { type == Type::Map };
    static constexpr bool is_object       { type == Type::Object };
    static constexpr bool is_pointer      { type == Type::Pointer };
    static constexpr bool is_owner        { not is_pointer or is_same_base_v<std::unique_ptr, T> or is_same_base_v<std::shared_ptr, T> };


    template <typename Object>
    static auto getEmptyEntry() {
        using Value = typename convert<Parent, std::remove_cv_t<Object>>::Value;
        return ::fon::getEmpty<Value>();
    }

    template<typename T2>
    auto operator%(T2& t) const {
        return mParent.operator%(t);
    }

    template <typename Object>
    auto visit(Object& object) const {
        convert<Parent, std::remove_cv_t<Object>> conv;
        if constexpr (is_convert) {
            return conv.access(*this, object);
        } else if constexpr (is_object) {
            conv.range(object, [&](auto& key, auto& value) {
                (*this) % value;
            }, [&](auto& value) {
                *this % value;
            });
        } else if constexpr (is_list or is_map) {
            conv.range(object, [&](auto& key, auto& value) {
                (*this) % value;
            });
        }
    }
    template <typename Object, typename CB>
    void visit(Object& object, CB cb) const {
        constexpr Type type {convert<Parent, T>::type};
        static_assert(type == Type::List || type == Type::Map, "Must be a map or a list");

        convert<Parent, std::remove_cv_t<Object>> conv;
        conv.range(object, cb);
    }

    template <typename Object, typename CB1, typename CB2>
    void visit(Object& object, CB1 cb1, CB2 cb2) const {
        convert<Parent, std::remove_cv_t<Object>> conv;

        constexpr Type type {convert<Parent, T>::type};
        static_assert(type == Type::List
                      || type == Type::Map
                      || type == Type::Object, "Must be a map, a list or an object");
        if constexpr (type == Type::Object) {
            conv.range(object, cb1, cb2);
        } else {
            conv.range(object, cb1);
        }
    }
};

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

    template<typename T>
    auto operator% (T& t) const {
        auto wrapper = NodeWrapper<Node, std::remove_const_t<T>>{*this};
        return cb(wrapper, t);
    }
};

}
