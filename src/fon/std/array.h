#pragma once

#include <array>

#include "../proxy.h"

namespace fon {

template <typename T, size_t S>
struct proxy<std::array<T, S>> {
    static constexpr void reflect(auto& visitor, auto& self) {
        visitor % fon::List{[&](size_t len) { // init write
        }, [&](auto& cb) { // write each element
            for (size_t i{0}; i < self.size(); ++i) {
                cb(i, self[i]);
            }
        }};
    }

    static constexpr void reflect(auto& visitor, auto const& self) {
        visitor % fon::List{[&]() { // init read
            return self.size();
        }, [&](auto& cb) { // read each element
            for (size_t i{0}; i < self.size(); ++i) {
                cb(i, self[i]);
            }
        }};
    }
};
}
