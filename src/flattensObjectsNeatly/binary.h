#pragma once

#include "utils.h"

#include <cstring>
#include <exception>
#include <optional>
#include <tuple>

#include "binarySerialize.h"
#include "binaryDeserialize.h"

namespace fon::binary {

namespace details {

template <typename T>
auto serialize(T const& _input) -> std::vector<std::byte> {
    auto& input = _input;

    std::vector<std::byte> buffer;
    buffer.reserve(1'000'000);

    fon::visit([&]<typename Node, typename ValueT> (Node& node, ValueT const& obj) {
        if constexpr (Node::is_convert) {
            Node::convert(node, obj);
        } else if constexpr (Node::is_value) {
            auto stack = SerializeStack{buffer};
            stack.setType(SerializeStack::Type::Value);
            if constexpr (std::is_fundamental_v<ValueT>) {
                stack = obj;
            } else if constexpr (std::is_same_v<std::string, ValueT>) {
                stack.set(obj.data(), obj.size());
            }
        } else if constexpr (Node::is_list) {
            auto frame = SerializeStack{buffer};
            frame.setType(SerializeStack::Type::Sequence);
            Node::range(obj, [&](auto& key, auto& value) {
                frame.push_back();
                node[key] % value;
            });
        } else if constexpr (Node::is_map) {
            auto frame = SerializeStack{buffer};
            frame.setType(SerializeStack::Type::Map);
            Node::range(obj, [&](auto& key, auto& value) {
                frame.push_back(serialize(key));
                node[key] % value;
            });
        } else if constexpr (Node::is_object) {
            auto frame = SerializeStack{buffer};
            Node::template range(obj, [&](auto& key, auto& value) {
                frame.setType(SerializeStack::Type::Map);
                frame.push_back(serialize(key));
                node[key] % value;
            }, [&](auto& value) {
                node % value;
            });
        }
    }, input);

    return buffer;
}

template <typename T>
auto deserialize(std::vector<std::byte> buffer, size_t startIndex=0) -> T {

    auto res = getEmpty<T>();

    if (buffer.size() == 0) {
        return res;
    }

    size_t currentIndex {startIndex};

    visit([&]<typename Node, typename ValueT>(Node& node, ValueT& obj) {

        //stack.back().
        //stack.push_back(

        /*DeserializeStack stack{buffer, 0};
        auto top = access_key(root, node);

        if (not top.IsDefined()) {
            return;
        }*/

        if constexpr (Node::is_convert) {
            Node::convert(node, obj);
        } else if constexpr (Node::is_value) {
            DeserializeStack frame{buffer, currentIndex};
            if constexpr (std::is_fundamental_v<ValueT>) {
                obj = frame.as<ValueT>();
            } else if constexpr (std::is_same_v<std::string, ValueT>) {
                obj.resize(frame.payloadSize());
                std::memcpy(obj.data(), frame.data<char>(), frame.payloadSize());
            }
        } else if constexpr (Node::is_list) {
            DeserializeStack frame{buffer, currentIndex};
            if (frame.type == DeserializeStack::Type::Sequence) {
                Node::reserve(obj, frame.entries);
                auto oldIndex = currentIndex;
                for (size_t idx{0}; idx < frame.entries; ++idx) {
                    currentIndex = frame.nextIndex();

                    auto value = Node::getEmpty();
                    node % value;
                    Node::emplace(obj, value);
                }
                currentIndex = oldIndex;
            }
        } else if constexpr (Node::is_map) {
            DeserializeStack frame{buffer, currentIndex};
            if (frame.type == DeserializeStack::Type::Map) {
                using Key   = typename Node::Key;
                Node::reserve(obj, frame.entries);
                auto oldIndex = currentIndex;
                for (auto i{0}; i < frame.entries; ++i) {
                    currentIndex = frame.nextIndex();
                    auto key = deserialize<Key>(buffer, currentIndex);
                    currentIndex = frame.nextIndex();
                    Node::emplace(node, obj, key);
                }
                currentIndex = oldIndex;
            }
        } else if constexpr (Node::is_object) {
            DeserializeStack frame{buffer, currentIndex};
            if (frame.type == DeserializeStack::Type::Map) {

                std::map<size_t, size_t> allEntries;
                for (auto i{0}; i < frame.entries; ++i) {
                    auto keyIndex  = frame.nextIndex();
                    auto valueIndex = frame.nextIndex();
                    allEntries[keyIndex] = valueIndex;
                }
                auto findEntry = [&](auto const& key) -> std::optional<size_t> {
                    auto skey = serialize(key);
                    for (auto [_key, _value] : allEntries) {
                        if (std::equal(skey.begin(), skey.end(), buffer.begin() + _key)) {
                            return _value;
                        }
                    }
                    return std::nullopt;
                };

                auto oldIndex = currentIndex;
                Node::range(obj, [&](auto& key, auto& value) {
                    auto index = findEntry(key);
                    if (index) {
                        currentIndex = index.value();
                        node[key] % value;
                    }
                }, [&](auto& value) {
                    node % value;
                });
                currentIndex = oldIndex;
            } else { // maybe direct
                Node::range(obj, [&](auto&, auto&) {
                    throw std::runtime_error("invalid structure");
                }, [&](auto& value) {
                    node % value;
                });
            }
        }
    }, res);


    return res;
}
}
using details::serialize;
using details::deserialize;

}
