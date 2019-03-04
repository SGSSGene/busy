#pragma once

#include <string_view>

namespace busy {

std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);

}