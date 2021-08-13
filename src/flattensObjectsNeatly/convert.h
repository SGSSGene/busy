#pragma once

#include "traits.h"

#include <array>
#include <deque>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <optional>

namespace fon {

enum class Type {
    None,
    Convertible,
    List,
    Map,
    Object,
    Pointer,
};


// converter functions
template <typename Node, typename T>
struct convert {
    static constexpr Type type = Type::None;
};


// convertible
template <typename Node, typename T>
    requires (std::is_enum_v<T>)
struct convert<Node, T> {
    static constexpr Type type = Type::Convertible;
    template <typename Node2>
    static void access(Node2& node, T& obj) {
        using UT = std::underlying_type_t<T>;
        auto val = static_cast<UT>(obj);
        node % val;
        obj = T {val};
    }
    template <typename Node2>
    static auto access(Node2& node, T const& obj) {
        using UT = std::underlying_type_t<T>;
        auto val = static_cast<UT>(obj);
        return node % val;
    }
};


// list types
template <template <typename...> typename C, typename T>
constexpr static bool is_sequence_v = is_any_of_v<C<T>,
    std::vector<T>, std::list<T>, std::deque<T>>;

template <typename Node, typename T, template <typename...> typename C>
    requires is_sequence_v<C, T>
struct convert<Node, C<T>> {
    static constexpr Type type = Type::List;
    using Key   = size_t;
    using Value = T;

    template <typename L, typename Self>
    static void range(Self& obj, L const& l) {
        std::size_t i{0};
        for (auto& e : obj) {
            l(i, e);
            ++i;
        }
    };

    static void reserve(C<T>& obj, size_t size) {
        if constexpr (std::is_same_v<std::vector<T>, C<T>>) {
            obj.reserve(size);
        }
    }
    static auto emplace(C<T>& obj, T e) {
        obj.push_back(e);
    }
};

template <template <typename...> typename C, typename T>
constexpr static bool is_set_v = is_any_of_v<C<T>,
    std::set<T>, std::unordered_set<T>>;


template <typename Node, typename T, template <typename...> typename C>
    requires is_set_v<C, T>
struct convert<Node, C<T>> {
    static constexpr Type type = Type::List;
    using Key   = size_t;
    using Value = T;
    template <typename L, typename Self>
    static void range(Self& obj, L const& l) {
        auto iter {begin(obj)};
        for (size_t i{0}; iter != end(obj); ++i, ++iter) {
            l(i, *iter);
        }
    }

    static void reserve(C<T>&, size_t) {}

    static auto emplace(C<T>& obj, T e) {
        obj.insert(std::move(e));
    }
};

// map types
template <template <typename...> typename C, typename Key, typename T>
constexpr static bool is_map_v = is_any_of_v<C<Key, T>,
    std::map<Key, T>, std::unordered_map<Key, T>>;

template <typename Node, typename TKey, typename T, template <typename...> typename C>
    requires is_map_v<C, TKey, T>
struct convert<Node, C<TKey, T>> {
    static constexpr Type type = Type::Map;
    using Key   = TKey;
    using Value = T;
    template <typename L, typename Self>
    static void range(Self& obj, L const& l) {
        for (auto& [key, value] : obj) {
            l(key, value);
        }
    }

    static void reserve(C<TKey, T>& obj, size_t size) {
        (void)obj;
        (void)size;
    }
    template <typename N2>
    static auto emplace(N2& node, C<TKey, T>& obj, TKey key) {
        auto value = getEmpty<Value>();
        node % value;
        obj.emplace(std::move(key), std::move(value));
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
};
}

template <typename Node, typename T>
    requires has_reflect_v<Node, T>
struct convert<Node, T> {
    static constexpr Type type = Type::Object;
    template <typename L1, typename L2, typename Self>
    static auto range(Self& obj, L1 const& l1, L2 const& l2) {
        auto visitor = helper::Visitor{l1, l2};
        T::reflect(visitor, obj);
    }
};

template <typename Node, typename T>
    requires has_proxy_v<Node, T>
struct convert<Node, T> {
    static constexpr Type type = Type::Object;
    template <typename L1, typename L2, typename Self>
    static auto range(Self& obj, L1 const& l1, L2 const& l2) {
        auto visitor = helper::Visitor{l1, l2};
        proxy<T>::reflect(visitor, obj);
    }
};


template <typename Node, typename T>
struct convert<Node, std::optional<T>> {
    static constexpr Type type = Type::List;
    using Key   = size_t;
    using Value = T;
    template <typename L, typename Self>
    static void range(Self& obj, L l) {
        if (obj.has_value()) {
            int i{0};
            l(i, *obj);
        }
    }

    static void reserve(std::optional<T>& obj, size_t i) {}

    static auto emplace(std::optional<T>& obj, T e) {
        obj.emplace(std::move(e));
    }
};

template <size_t TN>
struct l_apply_index {
    static constexpr size_t N {TN};
};

template <size_t N = 0, typename L, typename ...Args>
void l_apply(std::variant<Args...>& obj, L const& l) {
    if constexpr(N < sizeof...(Args)) {
        l(l_apply_index<N>{});
        l_apply<N+1>(obj, l);
    }
}
template <size_t N = 0, typename L, typename ...Args>
void l_apply(std::variant<Args...> const& obj, L const& l) {
    if constexpr(N < sizeof...(Args)) {
        l(l_apply_index<N>{});
        l_apply<N+1>(obj, l);
    }
}



template <typename Node, typename ...Args>
struct convert<Node, std::variant<Args...>> {
    static constexpr Type type = Type::Map;
    using Key   = size_t;
    template <typename L, typename Self>
    static void range(Self& obj, L l) {
        l_apply(obj, [&]<typename Index>(Index index) {
            if (Index::N == obj.index()) {
                size_t i = obj.index();
                l(i, std::get<Index::N>(obj));
            }
        });
    }

    static void reserve(std::variant<Args...>&, size_t) {}

    template <typename N2>
    static auto emplace(N2& node, std::variant<Args...>& obj, Key key) {
        if (key < 0 or key >= sizeof...(Args)) {
            throw std::runtime_error("accessing std::optional out of range");
        }
        l_apply(obj, [&]<typename Index>(Index index) {
            using Value = std::variant_alternative_t<Index::N, std::variant<Args...>>;
            if (Index::N == key) {
                obj = getEmpty<Value>();
                node % std::get<Index::N>(obj);
            }
        });
    }
};

// pointer types
template <typename Node, typename T>
struct convert<Node, T*> {
    static constexpr Type type = Type::Pointer;
    using Value = T;
};

template <typename Node, typename T>
struct convert<Node, std::unique_ptr<T>> {
    static constexpr Type type = Type::Pointer;
    using Value = T;
};

template <typename Node, typename T>
struct convert<Node, std::shared_ptr<T>> {
    static constexpr Type type = Type::Pointer;
    using Value = T;
};

// converter functions
template <typename T>
struct convert2 {
    T& value;
    convert2(T& value) : value{value} {}

    std::string toString() const {
        return value;
    }
    void fromString(std::string_view) {
        value = {};
    }
};

// converter functions
template <typename T>
    requires (std::is_same_v<nullptr_t, std::remove_cv_t<T>>)
struct convert2<T> {
    convert2(T&) {}

    std::string toString() const {
        return "";
    }
    void fromString(std::string_view) {}
};


template <typename T>
    requires (std::is_arithmetic_v<T>)
struct convert2<T> {
    T& value;
    convert2(T& value) : value{value} {}

    std::string toString() const {
        return std::to_string(value);
    }
    void fromString(std::string_view str) {
        //!TODO How to convert from a string view to a integer?
        value = std::stol(std::string{str});
    }
};

template <typename T>
    requires (std::is_same_v<std::string, std::remove_cv_t<T>>)
struct convert2<T> {
    T& value;
    convert2(T& value) : value{value} {}

    std::string toString() const {
        return value;
    }
    void fromString(std::string_view str) {
        value = str;
    }
};

template <>
struct convert2<char const*> {
    char const* const value;
    convert2(char const* const value) : value{value} {}

    std::string toString() const {
        return {value};
    }
    void fromString(std::string_view str) = delete;
};

template <>
struct convert2<char const* const&> {
    char const* const value;
    convert2(char const* const value) : value{value} {}

    std::string toString() const {
        return {value};
    }
    void fromString(std::string_view str) = delete;
};

template <typename Visitor, typename Object>
concept isConvertible = requires(Visitor visitor, Object object) {
    { convert<Visitor, Object>{}.access(visitor, object) };
};

template <typename Visitor, typename Object>
concept isListOrMap = requires(Visitor visitor, Object object) {
    { convert<Visitor, Object>{}.range(object, [](auto& key, auto& value) {
    }) };
};


template <typename Visitor, typename Object>
concept isObject = requires(Visitor visitor, Object object) {
    { convert<Visitor, Object>{}.range(object, [](auto& key, auto& value) {
    }, [](auto& value) {
    }) };
};


}
