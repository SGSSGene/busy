#pragma once

#include "Visitor.h"

#include <memory>
#include <string_view>
#include <type_traits>

namespace fon {

enum class ctor : uint8_t {};

template <typename T>
struct proxy;

template <typename Data> struct list_adapter;
template <typename Data> list_adapter(Data&, size_t) -> list_adapter<Data>;
template <typename Data> list_adapter(Data const&) -> list_adapter<Data const>;

template <typename Data> struct map_adapter;
template <typename Data> map_adapter(Data&, size_t) -> map_adapter<Data>;
template <typename Data> map_adapter(Data const&) -> map_adapter<Data const>;

template <typename Data> struct struct_adapter;
template <typename Data> struct_adapter(Data&) -> struct_adapter<Data>;
template <typename Data> struct_adapter(Data const&) -> struct_adapter<Data const>;

template <typename T>
concept has_reflect_v = requires(MockVisitor visitor, T t) {
    { T::reflect(visitor, t) };
};

template <typename T>
concept has_proxy_v = requires(MockVisitor visitor, T t) {
    { proxy<T>::reflect(visitor, t) };
};

template <typename T>
concept has_reflect_or_proxy_v = has_reflect_v<T> || has_proxy_v<T>;

template <typename T>
concept has_list_adapter_v = requires(T t, size_t l) {
    { list_adapter<T>{t, l} };
    { list_adapter<T const>{t} };
};

template <typename T>
concept has_map_adapter_v = requires(T t, size_t l) {
    { map_adapter<T>{t, l} };
    { map_adapter<T const>{t} };
};

template <typename T>
concept has_struct_adapter_v = requires(T t) {
    { struct_adapter<T>{t} };
    { struct_adapter<T const>{t} };
};

template <typename T>
auto getEmpty() -> T {
    static_assert(std::is_constructible_v<T, ctor> or std::is_constructible_v<T>,
        "object is not constructible");

    if constexpr (std::is_constructible_v<T>) {
        return T{};
    } else if constexpr (std::is_constructible_v<T, ctor> and not std::is_arithmetic_v<T>) {
        return T{ctor{}};
    } else {
        return T{};
    }
}
template <typename T>
auto makeEmpty() -> std::unique_ptr<T> {
    static_assert(std::is_constructible_v<T, ctor> or std::is_constructible_v<T>,
        "object is not constructible");

    if constexpr (std::is_constructible_v<T, ctor> and not std::is_arithmetic_v<T>) {
        return std::make_unique<T>(ctor{});
    } else if constexpr (std::is_constructible_v<T>) {
        return std::make_unique<T>();
    } else {
        return std::make_unique<T>();
    }
}


template <typename InitCB, typename EachCB>
struct List {
    InitCB initCb;
    EachCB eachCb;
};

template <typename CB1, typename CB2>
List(CB1, CB2) -> List<CB1, CB2>;


template <typename ...Args>
struct list_adapter<List<Args...>> {
    using Data = List<Args...>;
    Data& data;

    list_adapter(Data& data, size_t len)
        : data{data}
    {
        data.initCb(len);
    }

    void visit(auto cb) {
        data.eachCb(cb);
    }
};
template <typename ...Args>
struct list_adapter<List<Args...> const> {
    using Data = List<Args...>;
    Data const& data;
    size_t len;

    list_adapter(Data const& data)
        : data{data}
        , len{data.initCb()}
    {}
    auto size() const {
        return len;
    }

    void visit(auto cb) {
        data.eachCb(cb);
    }

};

template <typename InitCB, typename EachCB>
struct Map {
    InitCB initCb;
    EachCB eachCb;
};

template <typename CB1, typename CB2>
Map(CB1, CB2) -> Map<CB1, CB2>;

template <typename ...Args>
struct map_adapter<Map<Args...>> {
    using Data = Map<Args...>;
    Data& data;

    map_adapter(Data& data, size_t len)
        : data{data}
    {
        data.initCb(len);
    }

    void visit(auto cb) {
        data.eachCb(cb);
    }
};
template <typename ...Args>
struct map_adapter<Map<Args...> const> {
    using Data = Map<Args...>;
    Data const& data;
    size_t len;

    map_adapter(Data const& data)
        : data{data}
        , len{data.initCb()}
    {}
    auto size() const {
        return len;
    }

    void visit(auto cb) {
        data.eachCb(cb);
    }
};


namespace detail {

template <typename Cb, typename Key>
struct SubVisitor {
    Cb const& cb;
    Key key;
    SubVisitor(Cb const& _cb, Key _key)
        : cb{_cb}
        , key{_key}
    {}

    template <typename T>
    void operator%(T& obj) {
        cb(key, obj);
    }
};
template <typename Cb1, typename Cb2>
struct Visitor {
    Cb1 cb1;
    Cb2 cb2;

    Visitor(Cb1 const& _cb1, Cb2 const& _cb2)
        : cb1{_cb1}
        , cb2{_cb2}
    {};

    template <typename Key>
    auto operator[](Key key) {
        return SubVisitor{cb1, key};
    }
    auto operator[](std::string_view key) {
        return SubVisitor{cb1, key};
    }

    template <typename T>
    void operator%(T& obj) {
        cb2(obj);
    }
    template <typename T>
    void operator%(T&& obj) {
        cb2(obj);
    }
};
}


template <has_reflect_or_proxy_v Data>
struct struct_adapter<Data> {
    Data& data;

    struct_adapter(Data& data)
        : data{data}
    {}


    void visit(auto cb1, auto cb2) {
        detail::Visitor visitor{[&](auto& key, auto& value) {
            cb1(key, value);
        }, [&](auto& value) {
            cb2(value);
        }};
        if constexpr (fon::has_proxy_v<Data>) {
            proxy<Data>::reflect(visitor, data);
        } else {
            Data::reflect(visitor, data);
        }
    }

};

template <has_reflect_or_proxy_v Data>
struct struct_adapter<Data const> {
    Data const& data;

    struct_adapter(Data const& data)
        : data{data}
    {}

    auto size() const {
        return 0;
    }

    void visit(auto cb1, auto cb2) {
        detail::Visitor visitor{[&](auto& key, auto& value) {
            cb1(key, value);
        }, [&](auto& value) {
            cb2(value);
        }};
        if constexpr (fon::has_proxy_v<Data>) {
            proxy<Data>::reflect(visitor, data);
        } else {
            Data::reflect(visitor, data);
        }
    }
};

}
