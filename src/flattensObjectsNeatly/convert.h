#pragma once

#include "traits.h"

#include <array>
#include <deque>
#include <list>
#include <map>
#include <memory>
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

namespace fon {

enum class Type {
    None,
    Value,
    Convertible,
    DynamicList,
    Map,
    Object,
    Pointer,
};

// converter functions
template <typename Node, typename T>
struct convert;

// deduction guide
template<typename Node, typename T> convert(Node&, T&) -> convert<Node, T>;
template<typename Node, typename T> convert(Node&, T const&) -> convert<Node, T>;

// special "none" types
template <typename Node>
struct convert<Node, std::string_view> {
    static constexpr Type type = Type::None;
    struct Infos {};

    convert(Node&, std::string_view const&) {}
};


template<typename T>
concept ValueTypeConcept = std::is_arithmetic_v<T> or std::is_same_v<std::string, T>;

template <typename Node, ValueTypeConcept T>
struct convert<Node, T> {
    static constexpr Type type = Type::Value;
    struct Infos {};

    convert(Node&, T const&) {}
};


// convertible
template <typename T>
concept EnumConcept = std::is_enum_v<T>;

template <typename Node, EnumConcept T>
struct convert<Node, T> {
    static constexpr Type type = Type::Convertible;
    struct Infos {
        template <typename Node2>
        static void convert(Node2& node, T& obj) {
            using UT = std::underlying_type_t<T>;
            auto val = static_cast<UT>(obj);
            node % val;
            obj = T {val};
        }
        template <typename Node2>
        static auto convert(Node2& node, T const& obj) {
            using UT = std::underlying_type_t<T>;
            auto val = static_cast<UT>(obj);
            return node % val;
        }

    };
    convert(Node&, T const&) {}
};


// list types
template <template <typename...> typename C, typename T>
constexpr static bool is_sequence_v = is_any_of_v<C<T>,
    std::vector<T>, std::list<T>, std::deque<T>>;

template <typename Node, typename T, template <typename...> typename C>
    requires is_sequence_v<C, T>
struct convert<Node, C<T>> {
    static constexpr Type type = Type::DynamicList;
    struct Infos {
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

    template <typename Self>
    convert(Node& node, Self& obj) {
        auto iter {begin(obj)};
        for (size_t i{0}; iter != end(obj); ++i, ++iter) {
            node[i] % *iter;
        }
    }
};

template <template <typename...> typename C, typename T>
constexpr static bool is_set_v = is_any_of_v<C<T>,
    std::set<T>, std::unordered_set<T>>;


template <typename Node, typename T, template <typename...> typename C>
    requires is_set_v<C, T>
struct convert<Node, C<T>> {
    static constexpr Type type = Type::DynamicList;
    struct Infos {
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

    template <typename Self>
    convert(Node& node, Self& obj) {
        auto iter {begin(obj)};
        for (size_t i{0}; iter != end(obj); ++i, ++iter) {
            node[i] % *iter;
        }
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
    struct Infos {
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
            Value value = N2::getEmpty();
            node[key] % value;
            obj.emplace(std::move(key), std::move(value));
        }
    };
    template <typename Self>
    convert(Node& node, Self& obj) {
        for (auto& [key, value] : obj) {
            node[key] % value;
        }
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
    struct Infos {
        template <typename L1, typename L2, typename Self>
        static auto range(Self& obj, L1 const& l1, L2 const& l2) {
            auto visitor = helper::Visitor{l1, l2};
            T::reflect(visitor, obj);
        }
    };

    template <typename Self>
    convert(Node& node, Self& obj) {
        Self::reflect(node, obj);
    }
};

template <typename Node, typename T>
    requires has_proxy_v<Node, T>
struct convert<Node, T> {
    static constexpr Type type = Type::Object;
    struct Infos {
        template <typename L1, typename L2, typename Self>
        static auto range(Self& obj, L1 const& l1, L2 const& l2) {
            auto visitor = helper::Visitor{l1, l2};
            proxy<T>::reflect(visitor, obj);
        }
    };

    template <typename Self>
    convert(Node& node, Self& obj) {
        Self::reflect(node, obj);
    }
};


template <typename Node, typename T>
struct convert<Node, std::optional<T>> {
    static constexpr Type type = Type::DynamicList;
    struct Infos {
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

    template <typename Self>
    convert(Node& node, Self& obj) {
        if (obj.has_value()) {
            node[0] % obj.value();
        }
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
    struct Infos {
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
                    node[key] % std::get<Index::N>(obj);
                }
            });
        }
    };

    template <typename Self>
    convert(Node& node, Self& obj) {
        l_apply(obj, [&]<typename Index>(Index index) {
            if (Index::N == obj.index()) {
                node[obj.index()] % std::get<Index::N>(obj);
            }
        });
    }
};



template <size_t N = 0, typename L, typename ...Args>
void l_apply(std::tuple<Args...>& obj, L const& l) {
    if constexpr(N < sizeof...(Args)) {
        size_t n = N;
        l(n, std::get<N>(obj));
        l_apply<N+1>(obj, l);
    }
}
template <size_t N = 0, typename L, typename ...Args>
void l_apply(std::tuple<Args...> const& obj, L const& l) {
    if constexpr(N < sizeof...(Args)) {
        size_t n = N;
        l(n, std::get<N>(obj));
        l_apply<N+1>(obj, l);
    }
}

// pointer types
template <typename Node, typename T>
struct convert<Node, T*> {
    static constexpr Type type = Type::Pointer;
    struct Infos {
        using Value = T;
    };
    template <typename Self>
    convert(Node& node, Self& obj) {
        if (obj) {
            node % *obj;
        }
    }
};

template <typename Node, typename T>
struct convert<Node, std::unique_ptr<T>> {
    static constexpr Type type = Type::Pointer;
    struct Infos {
        using Value = T;
    };
    template <typename Self>
    convert(Node& node, Self& obj) {
        if (obj) {
            node % *obj;
        }
    }
};

template <typename Node, typename T>
struct convert<Node, std::shared_ptr<T>> {
    static constexpr Type type = Type::Pointer;
    struct Infos {
        using Value = T;
    };
    template <typename Self>
    convert(Node& node, Self& obj) {
        if (obj) {
            node % *obj;
        }
    }
};

}
