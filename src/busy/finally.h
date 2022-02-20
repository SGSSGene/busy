#pragma once

#include <functional>

namespace busy {

class finally {
    std::function<void()> cb;
public:
    template <typename CB>
    finally(CB _cb)
    : cb{_cb}
    {}

    ~finally() {
        if (cb) {
            cb();
        }
    }

    finally()                          = default;
    finally(finally const&)            = default;
    finally(finally&&)                 = default;
    auto operator=(finally const&) -> finally& = default;
    auto operator=(finally&&)      -> finally& = default;
};

}
