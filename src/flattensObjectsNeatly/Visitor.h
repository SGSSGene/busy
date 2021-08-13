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

private:
    template <typename T>
    auto _convert() {
        return convert<Visitor, std::remove_cv_t<T>>{};
    }
public:

    template <isObject<Visitor> T>
    void visit(isObject<Visitor> auto& obj) {
        _convert<T>().range(obj, [&](auto& key, auto& value) {
            (*this)[key] % value;
        }, [&](auto& value) {
            *this % value;
        });
    }

    template <isListOrMap<Visitor> T>
    void visit(T& obj) {
        _convert<T>().range(obj, [&](auto& key, auto& value) {
            (*this)[key] % value;
        });
    }

    /* Visiting a single value, object, list or map
     */
    template <isConvertible<Visitor> T>
    void visit(T& obj) {
        _convert<T>().access(*this, obj);
    }

    template <isListOrMap<Visitor> T>
    void visit(T& obj, auto cb) {
        _convert<T>().range(obj, cb);
    }

    template <isObject<Visitor> T>
    void visit(T& obj, auto cb1, auto cb2) {
        _convert<T>().range(obj, cb1, cb2);
    }


};

}
