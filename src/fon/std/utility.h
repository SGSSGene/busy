#pragma once

#include "../proxy.h"

#include <utility>

namespace fon {

template <typename T1, typename T2>
struct proxy<std::pair<T1, T2>> {
    static constexpr void reflect(auto& visitor, auto& self) {
        visitor[0] % self.first;
        visitor[1] % self.second;
    }
};

}
