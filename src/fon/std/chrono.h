#pragma once

#include <chrono>

#include "../proxy.h"

namespace fon {

template <typename Clock, typename Duration>
struct proxy<std::chrono::time_point<Clock, Duration>> {
    static constexpr void reflect(auto& visitor, auto& self) {
        decltype(self.time_since_epoch().count()) val{};
        visitor % val;
        self = std::chrono::time_point<Clock, Duration>(Duration(val));
    }

    static constexpr void reflect(auto& visitor, auto const& self) {
        auto val = self.time_since_epoch().count();
        visitor % val;
    }
};

template <typename Rep, typename Period>
struct proxy<std::chrono::duration<Rep, Period>> {
    static constexpr void reflect(auto& visitor, auto& self) {
        decltype(self.count()) val{};
        visitor % val;
        self = std::chrono::duration<Rep, Period>(val);
    }

    static constexpr void reflect(auto& visitor, auto const& self) {
        auto val = self.count();
        visitor % val;
    }
};

}
