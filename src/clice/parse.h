#pragma once

#include "Argument.h"
#include <fmt/format.h>
#include <iostream>

namespace clice {

inline auto parse(int argc, char const* const* argv) -> std::optional<std::string> {
    std::vector<ArgumentBase*> activeBases;

    auto completion = std::getenv("CLICE_COMPLETION") != nullptr;

    auto findRootArg = [&](std::string_view str) -> ArgumentBase* {
        for (auto arg : Register::getInstance().arguments) {
            if (arg->arg == str and arg->init) {
                return arg;
            }
        }
        return nullptr;
    };

    auto findActiveArg = [&](std::string_view str, ArgumentBase* base) -> ArgumentBase* {
        for (auto arg : base->arguments) {
            if (arg->arg == str and arg->init) {
                return arg;
            }
        }
        return nullptr;
    };


    bool allTrailing = false;
    for (int i{1}; i < argc; ++i) {
        // make suggestion about next possible tokens
        if (completion and i+1 == argc) {
            // single completion
            if (activeBases.size() and activeBases.back()->fromString and activeBases.back()->completion and (strlen(argv[i]) != 0 || argv[i][0] != '-')) {
                fmt::print("{}", *activeBases.back()->completion);
                continue;
            }
            auto options = std::vector<std::tuple<std::string, std::string>>{};
            for (auto bases : activeBases) {
                for (auto arg : bases->arguments) {
                    options.emplace_back(arg->arg, arg->desc);
                }
            }
            for (auto arg : Register::getInstance().arguments) {
                    options.emplace_back(arg->arg, arg->desc);
            }
            size_t longestWord = {};
            for (auto const& [arg, desc] : options) {
                longestWord = std::max(longestWord, arg.size());
            }
            //!TODO maybe we also want to show descriptions?

            std::ranges::sort(options);
            for (auto const& [arg, desc] : options) {
                fmt::print("{}\n", arg);
            }
            continue;
        }
        if (std::string_view{argv[i]} == "--" and !allTrailing) {
            allTrailing = true;
            continue;
        }

        [&]() {
            while (activeBases.size()) {
                if ((argv[i][0] != '-' or allTrailing) and activeBases.back()->fromString) {
                    activeBases.back()->fromString(argv[i]);
                    return;
                }
                if (auto arg = findActiveArg(argv[i], activeBases.back()); arg) {
                    arg->init();
                    activeBases.push_back(arg);
                    return;
                }
                activeBases.pop_back();
            }
            auto arg = findRootArg(argv[i]);
            if (arg) {
                arg->init();
                activeBases.push_back(arg);
                return;
            }
            throw std::runtime_error{std::string{"unexpected cli argument \""} + argv[i] + "\""};
        }();
    }
    // trigger all callbacks
    if (!completion) {
        auto& args = clice::Register::getInstance().arguments;
        auto f = std::function<void(std::vector<clice::ArgumentBase*>)>{};
        f = [&](auto const& args) {
            for (auto arg : args) {
                if (arg->cb) {
                    arg->cb();
                }
                f(arg->arguments);
            }
        };
        f(args);
    }


    return std::nullopt;
}

}
