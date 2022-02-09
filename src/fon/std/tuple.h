#pragma once

#include "../proxy.h"

#include <tuple>

namespace fon {

template <typename ...Args>
struct proxy<std::tuple<Args...>> {
    static constexpr void reflect(auto& visitor, auto& self) {
        reflect_impl<0>(visitor, self);
    }

    template <size_t N>
    static constexpr void reflect_impl(auto& visitor, auto& self) {
        if constexpr (N < sizeof...(Args)) {
            visitor[N] % std::get<N>(self);
            reflect_impl<N+1>(visitor, self);
        }
    }

};

}
