#pragma once

#include "Visitor.h"
#include "traits.h"

namespace fon {

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };

template <typename Target, typename CB, typename T>
void filter(CB cb, T const& t) {
    std::vector<std::string> keys;
    fon::visit([&]<typename Visitor, typename ValueT>(Visitor& visitor, ValueT& obj) {

        if constexpr (std::is_same_v<ValueT, Target const>
                      || std::is_same_v<ValueT, Target>
                      || std::is_base_of_v<Target, ValueT>
                      || std::is_base_of_v<Target const, ValueT>) {
            //!TODO How can we do this without const_cast and breaking many rules :-(
            cb(keys, const_cast<Target&>(static_cast<Target const&>(obj)));
        } else if constexpr (std::is_arithmetic_v<ValueT>
                      or std::is_same_v<std::string, ValueT>) {
            // leaf
        } else if constexpr (fon::has_list_adapter_v<ValueT>) {
            auto adapter = fon::list_adapter{obj};
            adapter.visit([&](size_t key, auto& value) {
                keys.emplace_back(std::to_string(key));
                visitor % value;
                keys.pop_back();
            });
        } else if constexpr (fon::has_map_adapter_v<ValueT>) {
            auto adapter = fon::map_adapter{obj};
            adapter.visit([&](auto& key, auto& value) {
                visitor % key;
                keys.emplace_back(std::to_string(key));
                visitor % value;
                keys.pop_back();
            });
        } else if constexpr (fon::has_struct_adapter_v<ValueT>) {
            auto adapter = fon::struct_adapter{obj};
            adapter.visit([&](auto& key, auto& value) {
                visitor % key;

                keys.emplace_back(key);
                visitor % value;
                keys.pop_back();
            }, [&](auto& value) {
                visitor % value;
            });
        } else if constexpr (std::is_pointer_v<ValueT>) {
            // leaf
        } else {
            []<bool flag = false>() {
                static_assert(fon::has_reflect_v<ValueT>, "fon: reflect or proxy missing (filter)");
            }();
        }
    }, t);
}

}
