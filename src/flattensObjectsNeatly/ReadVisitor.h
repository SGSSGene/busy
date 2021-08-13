#pragma once

#include "convert.h"

#include <string>
#include <string_view>
#include <variant>
#include <sstream>
#include <functional>


namespace fon {

template <typename Parent, typename T>
struct ReadVisitorWrapper {
    Parent const& mParent;

public:
    ReadVisitorWrapper()                                 = delete;
    ReadVisitorWrapper(ReadVisitorWrapper const&)            = delete;
    ReadVisitorWrapper(ReadVisitorWrapper&&)                 = default;
    ReadVisitorWrapper& operator=(ReadVisitorWrapper const&) = delete;
    ReadVisitorWrapper& operator=(ReadVisitorWrapper&&)      = default;
    ~ReadVisitorWrapper()                                = default;

    ReadVisitorWrapper(Parent const& _other)
        : mParent{_other}
    {}

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
        constexpr Type type {convert<Parent, T>::type};

        convert<Parent, std::remove_cv_t<Object>> conv;
        if constexpr (type == Type::Convertible) {
            return conv.access(*this, object);
        } else if constexpr (type == Type::Object) {
            conv.range(object, [&](auto& key, auto& value) {
                (*this) % value;
            }, [&](auto& value) {
                *this % value;
            });
        } else if constexpr (type == Type::List || type == Type::Map) {
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

    template <typename Key, typename T2>
    void operator()(Key const& key, T2 const& obj) {
        mParent(key, obj);
    }
    template <typename T2>
    void operator()(T2 const& obj) {
        mParent(nullptr, obj);
    }

};

template <typename Cb>
struct ReadVisitor {
protected:
    Cb const& cb;

public:
    ReadVisitor()                              = delete;
    ReadVisitor(ReadVisitor const&)            = delete;
    ReadVisitor(ReadVisitor&&)                 = default;
    ReadVisitor& operator=(ReadVisitor const&) = delete;
    ReadVisitor& operator=(ReadVisitor&&)      = delete;
    ~ReadVisitor()                             = default;

    ReadVisitor(Cb const& cb)
        : cb {cb}
    {}

    template <typename Key, typename T>
    void operator()(Key const& key, T const& obj) {
        using Self = decltype(*this);
        auto wrapper = ReadVisitorWrapper<Self, T>{*this};
        cb(wrapper, key, obj);
    }
    template <typename T>
    void operator()(T const& obj) {
        operator()(nullptr, obj);
    }
};

}
