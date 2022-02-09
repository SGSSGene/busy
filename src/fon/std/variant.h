#pragma once

#include "../proxy.h"

#include <variant>

namespace fon {

template <typename ...Args>
struct proxy<std::variant<Args...>> {
    static constexpr uint8_t Index = uint8_t{0};
    static constexpr uint8_t Value = uint8_t{1};

    static constexpr void reflect(auto& visitor, auto& self) {
        uint8_t index{};
        visitor[Index] % index;
        reflect_impl(visitor, self, index);
    }

    template <size_t N=0>
    static constexpr void reflect_impl(auto& visitor, auto& self, size_t index) {
        if constexpr (N < sizeof...(Args)) {
            if ( N == index ) {
                using T = std::variant_alternative_t<N, std::variant<Args...>>;
                auto v = getEmpty<T>();
                visitor[Value] % v;
                self = v;
            } else {
                reflect_impl<N+1>(visitor, self, index);
            }
        }
    }

    static constexpr void reflect(auto& visitor, auto const& self) {
        uint8_t index = self.index();
        visitor[Index] % index;
        std::visit([&](auto& obj) {
            visitor[Value] % obj;
        }, self);
    }
};

}
