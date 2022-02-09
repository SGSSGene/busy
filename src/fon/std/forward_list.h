#pragma once

#include "../proxy.h"

#include <forward_list>

namespace fon {

template <typename T, typename ...Args>
struct proxy<std::forward_list<T, Args...>> {
    static constexpr void reflect(auto& visitor, auto& self) {
        size_t size;
        visitor % fon::List{[&](size_t len) { // init write
            self.clear();
            size = len;
        }, [&](auto& cb) { // write each element
            auto iter = self.before_begin();
            for (size_t i{0}; i < size; ++i) {
                iter = self.emplace_after(iter, getEmpty<T>());
                cb(i, *iter);
            }
        }};
    }

    static constexpr void reflect(auto& visitor, auto const& self) {
        visitor % fon::List{[&]() { // init read
            // counts number of elements in std::forward_list (doesn't have size() method)
            size_t count{0};
            for ([[maybe_unused]]auto s : self) {
                count += 1;
            }
            return count;
        }, [&](auto& cb) { // read each element
            size_t i{0};
            for (auto const& e: self) {
                cb(i, e);
                ++i;
            }
        }};
    }
};

}
