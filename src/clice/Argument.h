#pragma once

#include "parseString.h"

#include <algorithm>
#include <any>
#include <filesystem>
#include <functional>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace clice {

struct ArgumentBase {
    ArgumentBase* parent{};
    std::string   arg;
    std::string   desc;
    std::vector<std::string> tags;
    std::optional<std::string> completion;

    std::function<void()> init;
    std::function<void(std::string_view)> fromString;
    std::function<void()> cb;

    ArgumentBase() = delete;
    ArgumentBase(ArgumentBase* parent);
    virtual ~ArgumentBase();
    ArgumentBase(ArgumentBase const&) = delete;
    ArgumentBase(ArgumentBase&&) = delete;
    auto operator=(ArgumentBase const&) -> ArgumentBase& = delete;
    auto operator=(ArgumentBase&&) -> ArgumentBase& = delete;

    std::vector<ArgumentBase*> arguments;
};

struct Register {
    std::vector<ArgumentBase*> arguments;

    static auto getInstance() -> Register& {
        static Register instance;
        return instance;
    }
};

template <typename T = nullptr_t, typename T2 = nullptr_t>
struct Argument {
    Argument<T2>* parent{};
    std::string arg;
    std::string desc;
    bool isSet{};
    T value{};
    mutable std::any anyType; // used if T is a callback
    std::function<void()> cb = [](){};
    std::optional<std::unordered_map<std::string, T>> mapping;
    std::vector<std::string> tags;

    operator bool() const {
        return isSet;
    }

    auto operator*() const -> decltype(auto) {
        if constexpr (std::is_invocable_v<T>) {
            using R = std::decay_t<decltype(value())>;
            if (!anyType.has_value()) {
                anyType = value();
            }
            return std::any_cast<R>(anyType);
        } else if constexpr (std::same_as<T, nullptr_t>) {
            []<bool type_available = false> {
                static_assert(type_available, "Can't dereference a flag");
            }();
        } else {
            return value;
        }
    }

    auto operator->() const -> auto const* {
        return &**this;
    }

    template <typename CB>
    auto run(CB _cb) -> nullptr_t {
        cb = _cb;
        return nullptr;
    }

    struct CTor {
        ArgumentBase arg;
        CTor(Argument& desc)
            : arg{desc.parent?&desc.parent->storage.arg:nullptr}
        {
            arg.arg  = desc.arg;
            arg.desc = desc.desc;
            if constexpr (std::same_as<std::filesystem::path, T>) {
                arg.completion = " -f ";
            } else if (desc.mapping) {
                std::string str;
                for (auto [key, value] : *desc.mapping) {
                    str += key + "\n";
                }
                arg.completion = str;
            }
            arg.tags = desc.tags;
            arg.init = [&]() {
                desc.isSet = true;
                arg.cb = desc.cb;
                if constexpr (std::same_as<nullptr_t, T>) {
                } else if constexpr (std::is_arithmetic_v<T>
                                     || std::same_as<std::string, T>
                                     || std::same_as<std::filesystem::path, T>
                                     || std::is_enum_v<T>) {
                    arg.fromString = [&](std::string_view s) {
                        if (desc.mapping) {
                            desc.value = desc.mapping->at(std::string{s});
                        } else {
                            desc.value = parseFromString<T>(s);
                        }
                        arg.fromString = nullptr;
                    };
                } else if constexpr (requires {{ std::declval<T>().push_back(std::declval<typename T::value_type>()) }; }) {
                    arg.fromString = [&](std::string_view s) {
                        if (desc.mapping) {
                            throw std::runtime_error("Type can't use mapping");
                        } else {
                            desc.value.push_back(parseFromString<typename T::value_type>(s));
                        }

                    };
                } else if constexpr (std::is_invocable_v<T>) {
                    arg.fromString = [&](std::string_view s) {
                        using RT = std::decay_t<decltype(desc.value())>;
                        desc.anyType = parseFromString<RT>(s);
                    };

                } else {
                    []<bool type_available = false> {
                        static_assert(type_available, "Type can't be used as a value type in clice::Argument");
                    }();
                }
            };
        }
    } storage{*this};
};

}
