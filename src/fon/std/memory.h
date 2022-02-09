#pragma once

#include "../proxy.h"

#include <memory>

namespace fon {

template <typename T>
struct proxy<std::unique_ptr<T>> {
    static constexpr void reflect(auto& visitor, auto& self) {
        visitor % fon::List{[&](size_t len) { // init write
            if (len == 0 and self) {
                self.reset();
            }  else if (len == 1 and !self){
                self.reset();
                self = makeEmpty<T>();
            }
        }, [&](auto& cb) { // write each element
            if (self) {
                cb(0, *self);
            }
        }};
    }

    static constexpr void reflect(auto& visitor, auto const& self) {
        visitor % fon::List{[&]() -> size_t { // init read
            return self?1:0;
        }, [&](auto& cb) { // read each element
            if (self) {
                cb(0, *self);
            }
        }};
    }
};

}
