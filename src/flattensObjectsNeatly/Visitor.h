#pragma once

#include "convert.h"

#include <string>
#include <string_view>
#include <variant>
#include <sstream>
#include <functional>


namespace fon {

template <typename Cb, typename Key=nullptr_t>
struct Visitor {
protected:
    Cb const& cb;
    Key const* const key;
public:
    Visitor()                          = delete;
    Visitor(Visitor const&)            = delete;
    Visitor(Visitor&&)                 = default;
    Visitor& operator=(Visitor const&) = delete;
    Visitor& operator=(Visitor&&)      = delete;
    ~Visitor()                         = default;

    template <typename T>
    static constexpr bool is_list() {
        return convert<Visitor, T>::type == Type::List;
    }
    template <typename T>
    static constexpr bool is_map() {
        return convert<Visitor, T>::type == Type::Map;
    }
    template <typename T>
    static constexpr bool is_object() {
        return convert<Visitor, T>::type == Type::Object;
    }


    Visitor(Cb const& cb)
        : cb {cb}
        , key {nullptr}
    {}

    Visitor(Cb const& cb, Key const& key)
        : cb {cb}
        , key {&key}
    {}

    template <typename Key2>
    auto operator[](Key2 const& key) {
        auto wrapper = Visitor<Cb, Key2>{cb, key};
        return wrapper;
    }

    template <typename T>
    auto operator%(T& obj) {
        return cb(*this, *key, obj);
    }

    template <isObject<Visitor> T>
    void visit(T& obj) {
        convert<Visitor, std::remove_cv_t<T>> conv;
        conv.range(obj, [&](auto& key, auto& value) {
            (*this)[key] % value;
        }, [&](auto& value) {
            *this % value;
        });
    }

    template <isListOrMap<Visitor> T>
    void visit(T& obj) {
        convert<Visitor, std::remove_cv_t<T>> conv;
        conv.range(obj, [&](auto& key, auto& value) {
            (*this)[key] % value;
        });
    }

    /* Visiting a single value, object, list or map
     */
    template <isConvertible<Visitor> T>
    void visit(T& obj) {
        constexpr Type type {convert<Visitor, T>::type};
        convert<Visitor, std::remove_cv_t<T>> conv;
        conv.access(*this, obj);
    }

    template <isListOrMap<Visitor> T, typename CB>
    void visit(T& obj, CB cb) const {
        convert<Visitor, std::remove_cv_t<T>> conv;
        conv.range(obj, cb);
    }

    template <isObject<Visitor> T, typename CB1, typename CB2>
    void visit(T& obj, CB1 cb1, CB2 cb2) const {
        convert<Visitor, std::remove_cv_t<T>> conv;
        conv.range(obj, cb1, cb2);
    }


};

template <typename Visitor, typename T>
constexpr bool is_list() {
    return convert<Visitor, T>::type == Type::List;
}
template <typename Visitor, typename T>
constexpr bool is_map() {
    return convert<Visitor, T>::type == Type::Map;
}
template <typename Visitor, typename T>
constexpr bool is_object() {
    return convert<Visitor, T>::type == Type::Object;
}


}
