#pragma once

#include "convert.h"

#include <chrono>

namespace fon {

// convertible
template <typename Node, class Clock, class Duration>
struct convert<Node, std::chrono::time_point<Clock, Duration>> {
    static constexpr Type type = Type::Convertible;
    template <typename Node2>
    static void access(Node2& node, std::chrono::time_point<Clock, Duration>& obj) {
        auto val = obj.time_since_epoch().count();
        node % val;
        obj = std::chrono::time_point<Clock, Duration>(Duration(val));
    }
    template <typename Node2>
    static auto access(Node2& node, std::chrono::time_point<Clock, Duration> const& obj) {
        auto val = obj.time_since_epoch().count();
        return node % val;
    }
};

template <typename Node, class Rep, class Period>
struct convert<Node, std::chrono::duration<Rep, Period>> {
    static constexpr Type type = Type::Convertible;
    template <typename Node2>
    static void access(Node2& node, std::chrono::duration<Rep, Period>& obj) {
        auto val = obj.count();
        node % val;
        obj = std::chrono::duration<Rep, Period>(val);
    }
    template <typename Node2>
    static auto access(Node2& node, std::chrono::duration<Rep, Period> const& obj) {
        auto val = obj.count();
        return node % val;
    }
};



}
