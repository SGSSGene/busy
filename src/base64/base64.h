#pragma once

#include <string_view>
#include <string>

namespace base64 {

auto encoded_len(std::string_view _decoded) -> std::size_t;
auto encode(std::string_view _decoded) -> std::string;
auto encoded_valid(std::string_view _encoded) -> bool;
auto decoded_len(std::string_view _encoded) -> std::size_t;
auto decode(std::string_view _encoded) -> std::string;

}
