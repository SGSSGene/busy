#pragma once

#include "traits.h"

#include <array>
#include <map>
#include <tuple>

namespace fon {

template <typename T>
struct proxy;

template <size_t N>
struct multi_element {
private:
    template <size_t I, typename Node, typename T>
    constexpr static void serialize_single_element(Node& node, T& t) {
        node[I] % std::get<I>(t);
    }

    template <typename Node, typename Self, typename T, auto... Is>
    constexpr static void serialize_all_elements(Node& node, Self& self, std::integer_sequence<T, Is...>) {
        (serialize_single_element<Is>(node, self), ...);
    }

public:
    template <typename Node, typename Self>
    constexpr static void reflect(Node& node, Self& self) {
        using Range = std::make_index_sequence<N>;
        serialize_all_elements(node, self, Range{});
    }

};


template <typename T, size_t N>
struct proxy<std::array<T, N>> {
    template <typename Node, typename Self>
    constexpr static void reflect(Node& node, Self& self) {
        multi_element<N>::reflect(node, self);
    }
};

template <typename... Args>
struct proxy<std::tuple<Args...>> {
    template <typename Node, typename Self>
    constexpr static void reflect(Node& node, Self& self) {
        multi_element<sizeof...(Args)>::reflect(node, self);
    }
};

template <typename T1, typename T2>
struct proxy<std::pair<T1, T2>> {
    template <typename Node, typename Self>
    constexpr static void reflect(Node& node, Self& self) {
        node[0] % self.first;
        node[1] % self.second;
    }
};

}
