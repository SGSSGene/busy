#pragma once

#include "utils.h"

#include <cassert>
#include <fmt/format.h>
#include <tuple>
#include <yaml-cpp/yaml.h>

namespace fon::yaml {

namespace details {

template <typename T>
auto serialize(T const& _input, YAML::Node start = {}) -> YAML::Node {
    auto& input = _input;

    std::map<void*, std::string> serializedShared; // helps tracking which shared ptr have already been serialized


    auto root = fon::visit([&]<typename Visitor, typename ValueT>(Visitor& visitor, ValueT const& obj) {
        YAML::Node top;

        // Interpret int8_t and uint8_t as ints not chars
        if constexpr (is_any_of_v<ValueT, int8_t, uint8_t>) {
            top = static_cast<int16_t>(obj);
        } else if constexpr (std::is_arithmetic_v<ValueT>
                      or std::is_same_v<std::string, ValueT>
                      or std::is_same_v<YAML::Node, ValueT>) {
            top = obj;
        } else if constexpr (std::is_same_v<ValueT, std::string_view>
                            or std::is_same_v<ValueT, char const*>) {
            top = std::string{obj};
        } else if constexpr (Visitor::is_list) {
            visitor.visit(obj, [&](auto& key, auto& value) {
                auto right = visitor % value;
                top.push_back(right);
            });
        } else if constexpr (Visitor::is_map) {
            visitor.visit(obj, [&](auto& key, auto& value) {
                auto left  = visitor % key;
                auto right = visitor % value;
                top[left] = right;
            });
        } else if constexpr (Visitor::is_object) {
            top = YAML::Node(YAML::NodeType::Map);
            visitor.visit(obj, [&](auto& key, auto& value) {
                auto left  = visitor % key;
                auto right = visitor % value;
                top[left] = right;
            }, [&](auto& value) {
                top = visitor % value;
            });
/*        } else if constexpr (Visitor::is_pointer) {
            fmt::print("name: {}\n", is_same_base_v<std::unique_ptr, ValueT>);
            if constexpr (is_same_base_v<std::unique_ptr, ValueT>) {
                top = visitor % *obj;
            } else if constexpr (is_same_base_v<std::shared_ptr, ValueT>) {
                using BaseType = typename Visitor::Value;
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
//                        serializedShared[mostDerived] = node->getPath();
                        visitor["data"] % *obj;
                    } else {
                        top["type"] = "secondary";
                        top["path"] = iter->second;
                    }
                }
            } else {
                //findPath(input, obj, [&](auto& node) {
                    //top = node->getPath();
                //});
            }*/
        } else {
            top = visitor.visit(obj);
        }
        return top;
    }, input);

    return root;
}

template <typename T>
auto serialize2(T const& _input, YAML::Node start = {}) -> YAML::Node {
    auto& input = _input;

    std::map<void*, std::string> serializedShared; // helps tracking which shared ptr have already been serialized


    auto root = fon::visit2([&]<typename Visitor, typename ValueT>(Visitor& visitor, ValueT const& obj) {
        YAML::Node top;

        // Interpret int8_t and uint8_t as ints not chars
        if constexpr (is_any_of_v<ValueT, int8_t, uint8_t>) {
            top = static_cast<int16_t>(obj);
        } else if constexpr (std::is_arithmetic_v<ValueT>
                      or std::is_same_v<std::string, ValueT>
                      or std::is_same_v<YAML::Node, ValueT>) {
            top = obj;
        } else if constexpr (std::is_same_v<ValueT, std::string_view>
                            or std::is_same_v<ValueT, char const*>) {
            top = std::string{obj};
        } else if constexpr (is_list<Visitor, ValueT>()) {
            visitor.visit(obj, [&](auto& key, auto& value) {
                auto right = visitor % value;
                top.push_back(right);
            });
        } else if constexpr (is_map<Visitor, ValueT>()) {
            visitor.visit(obj, [&](auto& key, auto& value) {
                auto left  = visitor % key;
                auto right = visitor % value;
                top[left] = right;
            });
        } else if constexpr (is_object<Visitor, ValueT>()) {
            top = YAML::Node(YAML::NodeType::Map);
            visitor.visit(obj, [&](auto& key, auto& value) {
                auto left  = visitor % key;
                auto right = visitor % value;
                top[left] = right;
            }, [&](auto& value) {
                top = visitor % value;
            });
/*        } else if constexpr (Visitor::is_pointer) {
            fmt::print("name: {}\n", is_same_base_v<std::unique_ptr, ValueT>);
            if constexpr (is_same_base_v<std::unique_ptr, ValueT>) {
                top = visitor % *obj;
            } else if constexpr (is_same_base_v<std::shared_ptr, ValueT>) {
                using BaseType = typename Visitor::Value;
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
//                        serializedShared[mostDerived] = node->getPath();
                        visitor["data"] % *obj;
                    } else {
                        top["type"] = "secondary";
                        top["path"] = iter->second;
                    }
                }
            } else {
                //findPath(input, obj, [&](auto& node) {
                    //top = node->getPath();
                //});
            }*/
        } else {
            top = visitor.visit(obj);
        }
        return top;
    }, [](auto key) { return nullptr; }, input);

    return root;
}



struct yaml_error : std::runtime_error {
    yaml_error(std::string s, YAML::Node const& node)
        : runtime_error(s + " in line " + std::to_string(node.Mark().line) + ":" + std::to_string(node.Mark().column) + " (" + std::to_string(node.Mark().pos) + ")")
    {}
};

struct StackGuard final {
    std::vector<YAML::Node>& stack;

    StackGuard(std::vector<YAML::Node>& stack, YAML::Node n)
        : stack{stack}
    {
        stack.push_back(n);
    }
    ~StackGuard() {
        stack.pop_back();
    }
};

template <typename T>
auto deserialize(YAML::Node root) -> T {

    auto res = getEmpty<T>();
    std::vector<YAML::Node> nodeStack{root};
    visit([&]<typename Visitor, typename ValueT>(Visitor& visitor, ValueT& obj) {
        auto top = nodeStack.back();

        if (not top.IsDefined()) {
            return;
        }

        try {
            if constexpr (std::is_same_v<YAML::Node, ValueT>) {
                obj = top;
            } else if constexpr (std::is_arithmetic_v<ValueT>
                      or std::is_same_v<std::string, ValueT>) {
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
            } else if constexpr (Visitor::is_list) {
                Visitor::reserve(obj, top.size());
                for (size_t idx{0}; idx < top.size(); ++idx) {
                    auto value = Visitor::template getEmptyEntry<ValueT>();
                    auto g = StackGuard{nodeStack, top[idx]};
                    visitor % value;
                    Visitor::emplace(obj, value);
                }
            } else if constexpr (Visitor::is_map) {
                using Key   = typename Visitor::Key;
                auto y_node = top;
                Visitor::reserve(obj, y_node.size());
                for (auto iter{y_node.begin()}; iter != y_node.end(); ++iter) {
                    auto key = deserialize<Key>(iter->first);
                    auto g = StackGuard{nodeStack, iter->second};
                    Visitor::emplace(visitor, obj, std::move(key));
                }
            } else if constexpr (Visitor::is_object) {
                visitor.visit(obj, [&](auto& key, auto& value) {
                    //!WORKAROUND for bug: https://github.com/jbeder/yaml-cpp/issues/979
                    auto fakeKey = [&]() {
                        YAML::Emitter emit;
                        emit << serialize(key);
                        return std::string{emit.c_str()};
                    }();
                    auto g = StackGuard{nodeStack, top[fakeKey]};
                    visitor % value;
                }, [&](auto& value) {
                    visitor % value;
                });

/*            } else if constexpr (Node::is_pointer) {
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
                }*/
            } else {
                visitor.visit(obj);
            }
        } catch(yaml_error const&) {
            throw;
        } catch(...) {
            std::throw_with_nested(yaml_error("error reading yaml file ", top));
        }
    }, res);

/*    visit([&]<typename Node, typename ValueT>(Node& node, ValueT& obj) {
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

    }, res);*/

    return res;
}
}
using details::serialize;
using details::deserialize;
using details::serialize2;

}

namespace fon {

// convertible
template <typename Node>
struct convert<Node, ::YAML::Node> {
    static constexpr Type type = Type::Convertible;
    template <typename Node2>
    static void access(Node2& node, YAML::Node& obj) {
        std::stringstream ss;
        ss << obj;
        auto val = ss.str();
        node % val;
        obj = val;
    }

    template <typename Node2>
    static void access(Node2& node, YAML::Node const& obj) {
        std::stringstream ss;
        ss << obj;
        auto val = ss.str();
        return node % val;
    }
};



}
