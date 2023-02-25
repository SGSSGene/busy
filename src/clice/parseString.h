#pragma once

#include <algorithm>
#include <filesystem>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>

namespace clice {

template <uint64_t maxValue, typename T>
auto parseSuffixHelper(std::string str, std::string_view suffix) -> T {
    if (str != suffix) {
        return 1;
    }
    if constexpr (maxValue >= uint64_t(std::numeric_limits<T>::max())) {
        throw std::runtime_error{"out of range"};
    } else {
        return maxValue;
    }
}

template<typename T>
auto parseSuffix(std::string_view suffix) -> std::optional<T> {
    auto ret = 1
        * parseSuffixHelper<1000, T>("k", suffix)
        * parseSuffixHelper<1024, T>("ki", suffix)
        * parseSuffixHelper<1000*1000, T>("m", suffix)
        * parseSuffixHelper<1024*1024, T>("mi", suffix)
        * parseSuffixHelper<1000*1000*1000, T>("g", suffix)
        * parseSuffixHelper<1024*1024*1024, T>("gi", suffix)
        * parseSuffixHelper<1000ull*1000*1000*1000, T>("t", suffix)
        * parseSuffixHelper<1024ull*1024*1024*1024, T>("ti", suffix)
        * parseSuffixHelper<1000ull*1000*1000*1000*1000, T>("p", suffix)
        * parseSuffixHelper<1024ull*1024*1024*1024*1024, T>("pi", suffix)
        * parseSuffixHelper<1000ull*1000*1000*1000*1000*1000, T>("e", suffix)
        * parseSuffixHelper<1024ull*1024*1024*1024*1024*1024, T>("ei", suffix)
    ;
    if (ret == 1) {
        return std::nullopt;
    }
    return {ret};
}

template<typename T>
auto parseFromString(std::string_view _str) -> T {
    auto str = std::string{_str};
    if constexpr (std::is_same_v<T, bool>) {
        std::ranges::transform(str, str.begin(), ::tolower);
        if (str == "true" or str == "1" or str == "yes") {
            return true;
        }
        if (str == "false" or str == "0" or str == "no") {
            return false;
        }
        throw std::runtime_error{std::string{"invalid boolean specifier \""} + str + "\""};
    } else if constexpr (std::is_same_v<T, std::string>) {
        return str;
    } else if constexpr (std::is_same_v<T, std::filesystem::path>) {
        return str;
    } else if constexpr (std::is_same_v<T, char>) {
        if (_str.size() != 1) {
            throw std::runtime_error{std::string{"invalid char specifier, must be exactly one char \""} + str + "\""};
        }
        return _str[0];
    } else if constexpr (std::numeric_limits<T>::is_exact) {
        // parse all integer-like types
        auto ret = T{};
        std::ranges::transform(str, str.begin(), ::tolower);
        auto base = int{0};
        char const* strBegin = str.data();
        char const* strEnd   = str.data() + str.size();
        std::size_t nextIdx=0;
        if (str.find("0b") == 0) {
            base=2;
            strBegin += 2;
        }
        try {
            if constexpr (std::is_unsigned_v<T>) {
                ret = std::stoull(strBegin, &nextIdx, base);
            } else {
                ret = std::stoll(strBegin, &nextIdx, base);
            }
        } catch (std::invalid_argument const&) {
            throw std::runtime_error{std::string{"not a valid integer \""} + str + "\""};
        }
        // if we didn't parse everything check if it has some known suffix
        if (static_cast<int>(nextIdx) != strEnd - strBegin) {
            if constexpr (not std::is_same_v<bool, T>) {
                auto suffix = std::string_view{strBegin + nextIdx};
                auto value = parseSuffix<T>(suffix);
                if (not value) {
                    throw std::runtime_error{std::string{"unknown integer suffix \""} + str + "\""};
                }
                ret *= value.value();
            }
        }
        return ret;

    } else if constexpr (std::is_enum_v<T>) {
        using UT = std::underlying_type_t<T>;
        auto ret = UT{};
        // parse everything else
        auto ss = std::stringstream{str};
        if (not (ss >> ret)) {
            throw std::runtime_error{std::string{"error parsing cli"}};
        }
        return T(ret);

    } else {
        // parse everything else
        auto ret = T{};
        auto ss = std::stringstream{str};
        if (not (ss >> ret)) {
            throw std::runtime_error{std::string{"error parsing cli"}};
        }
        // parse floats/doubles and convert if they are angles
        if constexpr (std::is_floating_point_v<T>) {
            if (not ss.eof()) {
                auto ending = std::string{};
                if (not (ss >> ending)) {
                    throw std::runtime_error{std::string{"invalid string \""} + str + "\""};
                }
                if (ending == "rad") {
                } else if (ending == "deg") {
                    ret = ret / 180. * std::numbers::pi;
                } else if (ending == "pi" or ending == "π") {
                    ret = ret * std::numbers::pi;
                } else if (ending == "tau" or ending == "τ") {
                    ret = ret * 2. * std::numbers::pi;
                } else {
                    throw std::runtime_error{std::string{"unknown suffix \""} + str + "\""};
                }
            }
        }

        if (not ss.eof()) {
            throw std::runtime_error{std::string{"error parsing cli"}};
        }
        return ret;
    }
}

}
