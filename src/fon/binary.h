#pragma once

#include "Visitor.h"
#include "proxy.h"
#include "traits.h"
#include "utils.h"

#include <cassert>
#include <cstring>

namespace fon::binary {

namespace details {

template <typename T>
auto serialize(T const& _input, std::vector<std::byte> buffer = {}) -> std::vector<std::byte> {
    fon::visit([&]<typename Visitor, typename ValueT>(Visitor& visitor, ValueT const& obj) {

        auto addValue = [&](auto const& value) {
            auto s = sizeof(value);
            buffer.resize(buffer.size() + s);
            std::memcpy(&buffer[buffer.size() - s], &value, s);
        };
        auto addContigous = [&](auto const& begin, size_t len) {
            buffer.resize(buffer.size() + len);
            std::memcpy(&buffer[buffer.size() - len], begin, len);
        };

        // Interpret all ints and floats
        if constexpr (std::is_arithmetic_v<ValueT>) {
            addValue(obj);
        } else if constexpr (std::is_same_v<std::string, ValueT>) {
            addValue(obj.size());
            addContigous(&obj[0], obj.size());
        } else if constexpr (std::is_same_v<ValueT, std::string_view>
                            or std::is_same_v<ValueT, char const*>) {
            auto s = std::string_view{obj};
            addValue(s.size());
            addContigous(&s[0], s.size());
        } else if constexpr (fon::has_list_adapter_v<ValueT>) {
            auto adapter = fon::list_adapter{obj};
            addValue(adapter.size());
            adapter.visit([&](size_t key, auto& value) {
                visitor % value;
            });
        } else if constexpr (fon::has_map_adapter_v<ValueT>) {
            auto adapter = fon::map_adapter{obj};
            addValue(adapter.size());
            adapter.visit([&](auto& key, auto& value) {
                visitor % key;
                visitor % value;
            });
        } else if constexpr (fon::has_struct_adapter_v<ValueT>) {
            auto adapter = fon::struct_adapter{obj};
            adapter.visit([&](auto& key, auto& value) {
                visitor % key;
                visitor % value;
            }, [&](auto& value) {
                visitor % value;
            });
        } else if constexpr (std::is_pointer_v<ValueT>) {
            using UT = std::remove_pointer_t<ValueT>;
            fon::filter<UT>([&](auto& keys, auto& obj2) {
                if (&obj2 == obj) {
                    std::string key;
                    for (auto const& k : keys) {
                        key += "/" + k;
                    }
                    addValue(key.size());
                    addContigous(&key[0], key.size());
                }
            }, _input);
        } else {
            []<bool flag = false>() {
                static_assert(fon::has_reflect_v<ValueT>, "fon: reflect or proxy missing (serialization)");
            }();
        }
    }, _input);

    return buffer;
}

template <typename T>
auto deserialize(std::vector<std::byte> buffer) -> T {

    size_t index{};

    auto readValue = [&](auto& obj) {
        if (index + sizeof(obj) > buffer.size()) {
            throw std::runtime_error("error deserializing (corrupted)");
        }
        std::memcpy(&obj, &buffer[index], sizeof(obj));
        index += sizeof(obj);
    };
    auto readSize = [&]() {
        size_t obj{};
        readValue(obj);
        return obj;
    };


    auto readContigous = [&](auto begin, size_t len) {
        if (index + len > buffer.size()) {
            throw std::runtime_error("error deserializing (corrupted)");
        }

        std::memcpy(begin, &buffer[index], len);
        index += len;
    };
    auto cmpContigous = [&](auto begin, size_t len) {
        if (index + len > buffer.size() or len > std::numeric_limits<size_t>::max()/2) {
            throw std::runtime_error("error deserializing (corrupted)");
        }

        auto res = std::memcmp(begin, &buffer[index], len);
        index += len;
        return res;
    };


    auto res = getEmpty<T>();

    std::vector<std::function<void()>> fixPointers;
    fon::visit([&]<typename Visitor, typename ValueT>(Visitor& visitor, ValueT& obj) {
        if constexpr (std::is_arithmetic_v<ValueT>) {
            readValue(obj);
        } else if constexpr (std::is_same_v<std::string, ValueT>) {
            auto len = readSize();
            obj.resize(len);
            readContigous(&obj[0], len);
        } else if constexpr (fon::has_list_adapter_v<ValueT>) {
            auto size = readSize();
            auto adapter = fon::list_adapter<ValueT>{obj, size};
            adapter.visit([&](size_t idx, auto& value) {
                visitor % value;
            });
        } else if constexpr (fon::has_map_adapter_v<ValueT>) {
            auto size = readSize();
            auto adapter = fon::map_adapter<ValueT>{obj, size};
            adapter.visit([&](auto& key, auto& value) {
                visitor % key;
                visitor % value;
            });
        } else if constexpr (fon::has_struct_adapter_v<ValueT>) {
            auto adapter = fon::struct_adapter{obj};
            adapter.visit([&](auto& key, auto& value) {
                 //!TODO currently the order has to be the same
                auto expectedKey = serialize(key);

                auto diff = cmpContigous(&expectedKey[0], expectedKey.size());
                if (diff != 0) {
                    throw std::runtime_error("found unexpected key when deserializing");
                }
                visitor % value;
            }, [&](auto& value) {
                visitor % value;
            });
        } else if constexpr (std::is_pointer_v<ValueT>) {
            auto keySize = readSize();
            std::string key;
            key.resize(keySize);
            readContigous(&key[0], keySize);
            fixPointers.push_back([&, key]() {
                using UT = std::remove_pointer_t<ValueT>;
                fon::filter<UT>([&]<typename Obj2>(auto& keys, Obj2 const& obj2) {
                    std::string key2;
                    for (auto const& k : keys) {
                        key2 += "/" + k;
                    }
                    if (key2 == key) {
                        obj = &const_cast<Obj2&>(obj2);
                    }
                }, res);
            });
            // must be done later
        } else {
            []<bool flag = false>() {
                static_assert(fon::has_reflect_v<ValueT>, "fon: reflect or proxy missing (deserialization)");
            }();
        }
    }, res);

    // add missing pointers
    for (auto& f : fixPointers) {
        f();
    }

    return res;
}

}

using details::serialize;
using details::deserialize;

}
