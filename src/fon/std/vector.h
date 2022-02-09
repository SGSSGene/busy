#pragma once

#include "../proxy.h"

#include <vector>

namespace fon {

template <typename T, typename ...Args>
struct proxy<std::vector<T, Args...>> {
    static constexpr void reflect(auto& visitor, auto& self) {
        visitor % fon::List{[&](size_t len) { // init write
            self.clear();
            self.resize(len, getEmpty<T>());
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
