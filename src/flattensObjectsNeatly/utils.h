#pragma once

#include "traits.h"
#include "convert.h"
#include "proxy.h"
#include "Node.h"
#include "Visitor.h"

namespace fon {

template <typename Cb, typename T>
auto visit(Cb cb, T& obj) {
    auto node = Node{cb};
    return node % obj;
}

template <typename Cb1, typename Cb2, typename T>
auto visit2(Cb1 cb1, Cb2 cb2, T& obj) {
    auto node = Visitor{cb1, cb2};
    return node % obj;
}

template <typename F, typename Cb, typename T>
void filter(Cb cb, T& obj) {
    visit([&]<typename Node, typename Value>(Node& node, Value& obj) {
        if constexpr(std::is_base_of_v<F, Value> or std::is_same_v<F, std::remove_const_t<Value>>) {
            cb(node, obj);
        } else if constexpr(std::is_polymorphic_v<F> and std::is_polymorphic_v<Value>) {
            auto* newObj = dynamic_cast<F*>(&obj);
            if (newObj) {
                cb(node, newObj);
            }
        } else if constexpr (Node::is_owner) {
            fon::convert(node, obj);
        }
    }, obj);
}

template <template <typename...> typename F, typename Cb, typename T>
void filter(Cb cb, T& obj) {
    visit([&]<typename Node, typename Value>(Node& node, Value& obj) {
        if constexpr (is_same_base_v<F, Value>) {
            cb(node, obj);
        } else if constexpr (Node::is_owner) {
            fon::convert(node, obj);
        }
    }, obj);
}

template <Type type, typename Cb, typename T>
void filter(Cb cb, T& obj) {
    visit([&]<typename Node>(Node& node, auto& obj) {
        if constexpr (Node::type == type) {
            cb(node, obj);
        } else if constexpr (Node::is_owner) {
            fon::convert(node, obj);
        }
    }, obj);
}

template <typename BaseType, typename T, typename L>
void findPath(T& input, BaseType* target, L l) {
    bool found{false};

    filter<BaseType>([&]<typename Node>(Node& node, BaseType& obj) {
        if (found) {
            return;
        }

        if (&obj == target) {
            l(node);
            found = true;
            return;
        }

        if constexpr (Node::is_owner) {
            fon::convert(node, obj);
        }
    }, input);
}

template <typename BaseType, typename T, typename L>
void findPath(T const& input, BaseType* target, L l) {
    bool found{false};

    filter<BaseType>([&]<typename Node>(Node& node, BaseType const& obj) {
        if (found) {
            return;
        }

        if (&obj == target) {
            l(node);
            found = true;
            return;
        }

        if constexpr (Node::is_owner) {
            fon::convert(node, obj);
        }
    }, input);
}

template <typename F, typename Cb1, typename Cb2, typename T>
void filter2(Cb1 cb1, Cb2 cb2, T& obj) {
    visit2([&]<typename Value>(auto& visitor, Value& obj) {
        if constexpr(std::is_base_of_v<F, Value> or std::is_same_v<F, std::remove_const_t<Value>>) {
            cb1(visitor, obj);
        } else if constexpr(std::is_polymorphic_v<F> and std::is_polymorphic_v<Value>) {
            auto* newObj = dynamic_cast<F*>(&obj);
            if (newObj) {
                cb1(visitor, newObj);
            } else {
                visitor.visit(obj);
            }
        } else {
            visitor.visit(obj);
        }
    }, cb2, obj);
}

template <template <typename...> typename F, typename Cb, typename T>
void filter2(Cb cb, T& obj) {
    visit2([&]<typename Value>(auto& visitor, Value& obj) {
        if constexpr (is_same_base_v<F, Value>) {
            cb(visitor, obj);
        }
    }, [](auto key) {}, obj);
}

template <Type type, typename Cb, typename T>
void filter2(Cb cb, T& obj) {
    visit2([&]<typename Node>(Node& node, auto& obj) {
        if constexpr (Node::type == type) {
            cb(node, obj);
        } else if constexpr (Node::is_owner) {
            fon::convert(node, obj);
        }
    }, obj);
}

template <typename BaseType, typename T, typename L>
void findPath2(T& input, BaseType* target, L l) {
    bool found{false};

    filter2<BaseType>([&]<typename Node>(Node& node, BaseType& obj) {
        if (found) {
            return;
        }

        if (&obj == target) {
            l(node);
            found = true;
            return;
        }

        if constexpr (Node::is_owner) {
            fon::convert(node, obj);
        }
    }, input);
}

template <typename BaseType, typename T, typename L>
void findPath2(T const& input, BaseType* target, L l) {
    bool found{false};

    filter2<BaseType>([&]<typename Node>(Node& node, BaseType const& obj) {
        if (found) {
            return;
        }

        if (&obj == target) {
            l(node);
            found = true;
            return;
        }

        if constexpr (Node::is_owner) {
            fon::convert(node, obj);
        }
    }, input);
}



template <typename BaseType, typename ...Args, typename T, typename L>
void findObj(T& input, std::string const& path, L l) {
    bool found{false};

    filter<BaseType>([&]<typename Node>(Node& node, auto& obj) {
        if (found) {
            return;
        }

/*        if (node->getPath() == path) {
            l(obj);
            found = true;
        }*/

        if constexpr (Node::is_owner) {
            fon::convert(node, obj);
        }
    }, input);
}



}
