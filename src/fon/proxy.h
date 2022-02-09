#pragma once

#include "traits.h"

#include <type_traits>
#include <vector>
#include <map>
#include <iostream>

namespace fon {

// converter functions
template <typename T>
    requires (std::is_enum_v<T>)
struct proxy<T> {
    static constexpr void reflect(auto& visitor, auto& self) {
        using UT = std::underlying_type_t<T>;
        auto val = static_cast<UT>(self);
        visitor % val;
        self = T {val};
    }
    static constexpr void reflect(auto& visitor, auto const& self) {
        using UT = std::underlying_type_t<T>;
        auto val = static_cast<UT>(self);
        return visitor % val;
    }
};

}
