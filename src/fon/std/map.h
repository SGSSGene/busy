#pragma once

#include "../proxy.h"

#include <map>

namespace fon {

template <typename Key, typename Value, typename ...Args>
struct proxy<std::map<Key, Value, Args...>> {
    static constexpr void reflect(auto& visitor, auto& self) {
        size_t totalCount {};
        visitor % fon::Map{[&](size_t len) { // init write
            totalCount = len;
            self.clear();
        }, [&](auto& cb) { // write each element
            for (size_t i{0}; i < totalCount; ++i) {
                Key key;
                Value value;
                cb(key, value);
                self.try_emplace(key, value);
            }
        }};
    }

    static constexpr void reflect(auto& visitor, auto const& self) {
        visitor % fon::Map{[&]() { // init read
            return self.size();
        }, [&](auto& cb) { // read each element
            for (auto const& [key, value] : self) {
                cb(key, value);
            }
        }};
    }
};

}
