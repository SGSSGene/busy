#pragma once

#include "../proxy.h"

#include <set>

namespace fon {

template <typename T, typename ...Args>
struct proxy<std::set<T, Args...>> {
    static constexpr void reflect(auto& visitor, auto& self) {
        size_t size;
        visitor % fon::List{[&](size_t len) { // init write
            self.clear();
            size = len;
        }, [&](auto& cb) { // write each element
            for (size_t i{0}; i < size; ++i) {
                auto e = getEmpty<T>();
                cb(i, e);
                self.emplace(e);
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
