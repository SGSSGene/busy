#pragma once

#include "utils.h"

#include <tuple>
#include <yaml-cpp/yaml.h>

namespace fon::yaml {

namespace details {

template <typename Key>
auto access_yaml_key(YAML::Node parent, Key key) -> YAML::Node {
    if constexpr(std::is_same_v<Key, std::string> or std::is_arithmetic_v<Key>) {
        return parent[key];
    } else {
        return parent[std::string{key}];
    }
}

template <size_t N, typename R, typename L, typename ...Args>
auto acc_tuple(std::tuple<Args...> const& tuple, L const& l, R&& res) {
    if constexpr (N < sizeof...(Args)) {
        return acc_tuple<N+1>(tuple, l, l(std::get<N>(tuple), std::forward<R>(res)));
    } else {
        return res;
    }
}

template <typename R, typename L, typename ...Args>
auto acc_tuple(R&& res, std::tuple<Args...> const& tuple, L const& l) {
    return acc_tuple<0>(tuple, l, std::forward<R>(res));
}

template <typename Node>
auto access_key(YAML::Node root, Node& node) {
    return acc_tuple(root, node->getFullKey(), [&](auto key, auto&& parent) {
        return access_yaml_key(std::forward<decltype(parent)>(parent), key);
    });
}



template <typename T>
auto serialize(T const& _input, YAML::Node start = {}) -> YAML::Node {
    auto& input = const_cast<T&>(_input);

    YAML::Node root;
    root[""] = start; // we need a level of indirection, so the ref mechanism of yaml-cpp works properly

    std::map<void*, std::string> serializedShared; // helps tracking which shared ptr have already been serialized

    fon::visit([&](auto& node, auto& obj) {
        using Node   = std::decay_t<decltype(node)>;
        using ValueT = std::decay_t<decltype(obj)>;

        auto top = access_key(root[""], node);

        if constexpr (std::is_same_v<YAML::Node, ValueT>) {
            top = obj;
        } else if constexpr (Node::is_convert) {
            Node::convert(node, obj);
        } else if constexpr (Node::is_value) {
            top = obj;
        } else if constexpr (Node::is_map or Node::is_list) {
            if constexpr (Node::is_map) { // !TODO Workaround, forces yaml maps
                top[1] = 0;
                top.remove(1);
            }
            Node::range(obj, [&](auto& key, auto& value) {
                node[key] % value;
            });

        } else if constexpr (Node::is_object) {
            bool forceMap = false;
            Node::range(obj, [&](auto& key, auto& value) {
                forceMap = true;
                node[key] % value;
            }, [&](auto& value) {
                node % value;
            });
            if (not top.IsMap() and forceMap) {
                // !TODO Workaround, forces yaml maps
                top["langerstringderhoffentlichsonichtvorkommtvielglueck"] = 0;
                top.remove("langerstringderhoffentlichsonichtvorkommtvielglueck");
            }

        } else if constexpr (Node::is_pointer) {
            if constexpr (is_same_base_v<std::unique_ptr, ValueT>) {
                node % *obj;
            } else if constexpr (is_same_base_v<std::shared_ptr, ValueT>) {
                using BaseType = typename Node::Value;
                if (obj) {
                    auto mostDerived = [&] {
                        if constexpr (std::is_polymorphic_v<BaseType>) {
                            return dynamic_cast<void*>(obj.get());
                        } else {
                            return obj.get();
                        }
                    }();
                    auto iter = serializedShared.find(mostDerived);
                    if (iter == serializedShared.end()) {
                        top["type"] = "primary";
                        serializedShared[mostDerived] = node->getPath();
                        node["data"] % *obj;
                    } else {
                        top["type"] = "secondary";
                        top["path"] = iter->second;
                    }
                }
            } else {
                findPath(input, obj, [&](auto& node) {
                    top = node->getPath();
                });
            }
        }
    }, input);

    return root[""];
}


struct yaml_error : std::runtime_error {
    yaml_error(std::string s, YAML::Node const& node)
        : runtime_error(s + " in line " + std::to_string(node.Mark().line) + ":" + std::to_string(node.Mark().column) + " (" + std::to_string(node.Mark().pos) + ")")
    {}    
};

template <typename T>
auto deserialize(YAML::Node root) -> T {

    auto res = getEmpty<T>();
    visit([&](auto& node, auto& obj) {
        using Node   = std::decay_t<decltype(node)>;
        using ValueT = std::decay_t<decltype(obj)>;

        auto top = access_key(root, node);

        if (not top.IsDefined()) {
            return;
        }

        try {
            if constexpr (std::is_same_v<YAML::Node, ValueT>) {
                obj = top;
            } else if constexpr (Node::is_convert) {
                Node::convert(node, obj);
            } else if constexpr (Node::is_value) {
                if constexpr (is_any_of_v<ValueT, int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t>) {
                    auto v = top.template as<int64_t>();
                    if (v < std::numeric_limits<ValueT>::min() or v > std::numeric_limits<ValueT>::max()) {
                        throw std::runtime_error("value out of range");
                    }
                    obj = v;
                } else {
                    // !TODO no check, we just hope it works
                    obj = top.template as<ValueT>();
                }
            } else if constexpr (Node::is_list) {
                Node::reserve(obj, top.size());
                for (size_t idx{0}; idx < top.size(); ++idx) {
                    Node::emplace(node, obj, idx);
                }
            } else if constexpr (Node::is_map) {
                using Key   = typename Node::Key;
                auto y_node = top;
                Node::reserve(obj, y_node.size());
                for (auto iter{y_node.begin()}; iter != y_node.end(); ++iter) {
                    auto key = iter->first.template as<Key>();
                    Node::emplace(node, obj, key);
                }
            } else if constexpr (Node::is_pointer) {
                using BaseType = typename Node::Value;
                if constexpr (is_same_base_v<std::unique_ptr, ValueT>) {
                    if (not obj) {
                        obj = std::make_unique<BaseType>(Node::getEmpty());
                    }
                    node % *obj;
                } else if constexpr (is_same_base_v<std::shared_ptr, ValueT>) {
                    if (top["type"].IsDefined() and top["type"].template as<std::string>() == "primary") {
                        obj = std::make_shared<BaseType>(Node::getEmpty());
                        node["data"] % *obj;
                    }
                }
            } else if constexpr (Node::is_object) {
                Node::range(obj, [&](auto& key, auto& value) {
                    node[key] % value;
                }, [&](auto& value) {
                    node % value;
                });
            }
        } catch(yaml_error const&) {
            throw;
        } catch(...) {
            std::throw_with_nested(yaml_error("error reading yaml file ", top));
        }
    }, res);

    visit([&](auto& node, auto& obj) {
        using Node   = std::decay_t<decltype(node)>;
        using ValueT = std::decay_t<decltype(obj)>;

        auto top = access_key(root, node);

        try {
            if constexpr (Node::is_pointer) {
                using BaseType = typename Node::Value;
                if constexpr (not Node::is_owner) {
                    if (not top.IsDefined()) {
                        return;
                    }
                    findObj<BaseType>(res, top.template as<std::string>(), [&](auto& _obj) {
                        obj = &_obj;
                    });
                } else if constexpr (is_same_base_v<std::shared_ptr, ValueT>) {
                    if (top["type"].IsDefined() and top["type"].template as<std::string>() == "secondary") {
                        findObj<ValueT>(res, top["path"].template as<std::string>(), [&](auto& _obj) {
                            obj = _obj;
                        });
                    }
                } else {
                    fon::convert(node, obj);
                }
            } else {
                fon::convert(node, obj);
            }
        } catch(yaml_error const&) {
            throw;
        } catch(...) {
            std::throw_with_nested(yaml_error("error reading yaml file ", top));
        }

    }, res);

    return res;
}
}
using details::serialize;
using details::deserialize;

}

namespace fon {

// convertible
template <typename Node>
struct convert<Node, ::YAML::Node> {
    static constexpr Type type = Type::Convertible;
    struct Infos {
        template <typename Node2>
        static void convert(Node2& node, YAML::Node& obj) {
            std::stringstream ss;
            ss << obj;
            auto val = ss.str();
            node % val;
            obj = val;
        }
    };
    convert(Node&, YAML::Node&) {}

};



}
