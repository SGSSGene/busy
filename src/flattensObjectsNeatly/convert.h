#pragma once

#include "traits.h"

#include <array>
#include <deque>
#include <forward_list>
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
#include <variant>
#include <vector>

namespace fon {

enum class Type {
    None,
    Value,
    Convertible,
    List,
    Map,
    Object,
    Pointer,
};

// converter functions
template <typename Node, typename T>
struct convert;

// deduction guide
template<typename Node, typename T> convert(Node&, T&) -> convert<Node, T>;

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

    convert(Node&, T&) {}
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
    };
    convert(Node&, T&) {}
};


// list types
template <template <typename...> typename C, typename T>
constexpr static bool is_sequence_v = is_any_of_v<C<T>,
    std::vector<T>, std::list<T>, std::deque<T>, std::forward_list<T>>;

template <typename Node, typename T, template <typename...> typename C>
requires is_sequence_v<C, T>
struct convert<Node, C<T>> {
    static constexpr Type type = Type::List;
    struct Infos {
        using Key   = size_t;
        using Value = T;

        template <typename L>
        static void range(C<T>& obj, L const& l) {
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
        template <typename N2>
        static auto emplace(N2& node, C<T>& obj, Key key) {
            if constexpr (std::is_same_v<std::forward_list<T>, C<T>>) {
                if (obj.empty()) {
                    obj.push_front(N2::getEmpty());
                    node[key] % obj.front();
                } else {
                    auto iter = obj.insert_after(next(begin(obj), key-1), N2::getEmpty());
                    node[key] % *iter;
                }
            } else {
                obj.push_back(N2::getEmpty());
                node[key] % obj.back();
            }
        }
    };

    convert(Node& node, C<T>& obj) {
        auto iter {begin(obj)};
        for (size_t i{0}; iter != end(obj); ++i, ++iter) {
            node[i] % *iter;
        }
    }
};

template <typename Node, typename T, size_t N>
struct convert<Node, std::array<T, N>> {
    static constexpr Type type = Type::List;
    struct Infos {
        using Key   = size_t;
        template <typename L>
        static void range(std::array<T, N>& obj, L const& l) {
            std::size_t i{0};
            for (std::size_t i{0}; i < N; ++i) {
                l(i, obj[i]);
            }
        };

        static void reserve(std::array<T, N>&, size_t) {}
        template <typename N2>
        static auto emplace(N2& node, std::array<T, N>& obj, Key key) {
            if (key >= N or key < 0) {
                throw std::runtime_error("accessing array out of range");
            }
            node[key] % obj[key];
        }
    };

    convert(Node& node, std::array<T, N>& obj) {
        for (size_t i{0}; i < N; ++i) {
            node[i] % obj.at(i);
        }
    }
};

template <template <typename...> typename C, typename T>
constexpr static bool is_set_v = is_any_of_v<C<T>,
    std::set<T>, std::unordered_set<T>>;


template <typename Node, typename T, template <typename...> typename C>
requires is_set_v<C, T>
struct convert<Node, C<T>> {
    static constexpr Type type = Type::List;
    struct Infos {
        using Key   = size_t;
        template <typename L>
        static void range(C<T>& obj, L const& l) {
            auto iter {begin(obj)};
            for (size_t i{0}; iter != end(obj); ++i, ++iter) {
                //!TODO this const cast needs to go away
                auto& entry = const_cast<T&>(*iter);
                l(i, entry);
            }
        }

        static void reserve(C<T>&, size_t) {}

        template <typename N2>
        static auto emplace(N2& node, C<T>& obj, Key key) {
            auto value = getEmpty<T>();
            node[key] % value;
            obj.insert(std::move(value));
        }
    };

    convert(Node& node, C<T>& obj) {
        auto iter {begin(obj)};
        for (size_t i{0}; iter != end(obj); ++i, ++iter) {
            //!TODO this const cast needs to go away
            auto& entry = const_cast<T&>(*iter);
            node[i] % entry;
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
        template <typename L>
        static void range(C<TKey, T>& obj, L const& l) {
            for (auto& [key, value] : obj) {
                l(key, value);
            }
        };

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
    convert(Node& node, C<TKey, T>& obj) {
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
    auto operator%(T& obj) {
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

    auto operator[](std::string_view key) {
        return SubVisitor{cb1, key};
    }

    template <typename T>
    auto operator%(T& obj) {
        cb2(obj);
    }
};
}

// object types
template <typename Node, typename T>
requires has_ser_v<Node, T>
struct convert<Node, T> {
    static constexpr Type type = Type::Object;
    struct Infos {
        template <typename L1, typename L2>
        static auto range(T& obj, L1 const& l1, L2 const& l2) {
            auto visitor = helper::Visitor{l1, l2};
            obj.serialize(visitor);
        };

    };

    convert(Node& node, T& obj) {
        obj.serialize(node);
    }
};

template <typename Node, typename T>
requires has_reflect_v<Node, T>
struct convert<Node, T> {
    static constexpr Type type = Type::Object;
    struct Infos {
        template <typename L1, typename L2>
        static auto range(T& obj, L1 const& l1, L2 const& l2) {
            auto visitor = helper::Visitor{l1, l2};
            std::decay_t<T>::reflect(visitor, obj);
        };
    };

    convert(Node& node, T& obj) {
        std::decay_t<T>::reflect(node, obj);
    }
};


template <typename Node, typename T>
struct convert<Node, std::optional<T>> {
    static constexpr Type type = Type::List;
    struct Infos {
        using Key   = size_t;
        template <typename L>
        static void range(std::optional<T>& obj, L l) {
            if (obj.has_value()) {
                int i{0};
                l(i, *obj);
            }
        };

        static void reserve(std::optional<T>& obj, size_t i) {
            if (i == 0) {
                obj = std::nullopt;
            } else {
                obj = getEmpty<T>();
            }
        }
        template <typename N2>
        static auto emplace(N2& node, std::optional<T>& obj, Key key) {
            if (key < 0 or key >= 1) {
                throw std::runtime_error("accessing std::optional out of range");
            }
            node[0] % obj.value();
        }
    };

    convert(Node& node, std::optional<T>& obj) {
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


template <typename Node, typename ...Args>
struct convert<Node, std::variant<Args...>> {
    static constexpr Type type = Type::Map;
    struct Infos {
        using Key   = size_t;
        template <typename L>
        static void range(std::variant<Args...>& obj, L l) {
            l_apply(obj, [&]<typename Index>(Index index) {
                if (Index::N == obj.index()) {
                    size_t i = obj.index();
                    l(i, std::get<Index::N>(obj));
                }
            });
        };

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

    convert(Node& node, std::variant<Args...>& obj) {
        l_apply(obj, [&]<typename Index>(Index index) {
            if (Index::N == obj.index()) {
                node[obj.index()] % std::get<Index::N>(obj);
            }
        });
    }
};



template <typename Node, typename T1, typename T2>
struct convert<Node, std::pair<T1, T2>> {
    static constexpr Type type = Type::List;
    struct Infos {
        using Key   = size_t;
        template <typename L>
        static void range(std::pair<T1, T2>& obj, L l) {
            int i{-1};
            l(++i, obj.first);
            l(++i, obj.second);
        };

        static void reserve(std::pair<T1, T2>&, size_t) {}

        template <typename N2>
        static auto emplace(N2& node, std::pair<T1, T2>& obj, Key key) {
            if (key < 0 or key >= 2) {
                throw std::runtime_error("accessing std::pair out of range");
            }
            if (key == 0) {
                node[key] % obj.first;
            } else {
                node[key] % obj.second;
            }
        }
    };

    convert(Node& node, std::pair<T1, T2>& obj) {
        node[0] % obj.first;
        node[1] % obj.second;
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



template <typename Node, typename... Args>
struct convert<Node, std::tuple<Args...>> {
    static constexpr Type type = Type::List;
    struct Infos {
        using Key   = size_t;
        template <typename L>
        static void range(std::tuple<Args...>& obj, L l) {
            l_apply(obj, l);
        };

        static void reserve(std::tuple<Args...>&, size_t) {}
        template <typename N2>
        static auto emplace(N2& node, std::tuple<Args...>& obj, Key key) {
            if (key < 0 or key >= sizeof...(Args)) {
                throw std::runtime_error("accessing std::tuple out of range");
            }
            l_apply(obj, [&](size_t i, auto& value) {
                if (i == key) {
                    node[key] % value;
                }
            });
        }
    };

    convert(Node& node, std::tuple<Args...>& obj) {
        l_apply(obj, [&](size_t i, auto& value) {
            node[i] % value;
        });
    }
};




// pointer types
template <typename Node, typename T>
struct convert<Node, T*> {
    static constexpr Type type = Type::Pointer;
    struct Infos {
        using Value = T;
    };
    convert(Node& node, T*& obj) {
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
    convert(Node& node, std::unique_ptr<T>& obj) {
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
    convert(Node& node, std::shared_ptr<T>& obj) {
        if (obj) {
            node % *obj;
        }
    }
};

}
