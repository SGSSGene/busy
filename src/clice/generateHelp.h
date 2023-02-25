#pragma once

#include "Argument.h"

#include <fmt/format.h>

namespace clice {
inline auto generateHelp() -> std::string {
    auto ret = std::string{};

    auto& args = clice::Register::getInstance().arguments;
    auto f = std::function<void(std::vector<clice::ArgumentBase*>, std::string)>{};
    f = [&](auto const& args, std::string ind) {
        size_t longestWord = {};
        for (auto arg : args) {
            longestWord = std::max(longestWord, arg->arg.size());
        }

        for (auto arg : args) {
            if (arg->arg.empty() or arg->arg[0] == '-') continue;
            ret = ret + fmt::format("{}{:<{}} - {}\n", ind, arg->arg, longestWord, arg->desc);
            f(arg->arguments, ind + "  ");
        }
        for (auto arg : args) {
            if (arg->arg.empty() or arg->arg[0] != '-') continue;
            ret = ret + fmt::format("{}{:<{}}    - {}\n", ind, arg->arg, longestWord, arg->desc);
            f(arg->arguments, ind + "  ");
        }

    };
    f(args, "");
    return ret;
}
}
