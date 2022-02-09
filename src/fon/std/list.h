#pragma once

#include "../proxy.h"

#include <list>

namespace fon {

template <typename T, typename ...Args>
struct proxy<std::list<T, Args...>> {
    static constexpr void reflect(auto& visitor, auto& self) {
        visitor % fon::List{[&](size_t len) { // init write
            self.clear();
            self.resize(len, getEmpty<T>());
        }, [&](auto& cb) { // write each element
            auto iter = self.begin();
            for (size_t i{0}; i < self.size(); ++i) {
                cb(i, *iter);
                ++iter;
            }
        }};
    }

    static constexpr void reflect(auto& visitor, auto const& self) {
        visitor % fon::List{[&]() { // init read
            return self.size();
        }, [&](auto& cb) { // read each element
            auto iter = self.begin();
            for (size_t i{0}; i < self.size(); ++i) {
                cb(i, *iter);
                ++iter;
            }
        }};
    }
};
}
