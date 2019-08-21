#include "base64.h"

#include <array>
#include <cstring>

namespace base64 {

namespace {
constexpr const auto base64_chars = std::string_view {
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	"abcdefghijklmnopqrstuvwxyz"
	"0123456789+/"
};

auto is_base64(char c) -> bool {
	return (std::isalnum(c) || (c == '+') || (c == '/'));
}

//!TODO must be unsigned to have correct shifting semantics
void encode_block(char const* _input, char* _output) {
	_output[0] = base64_chars[( _input[0] & 0xfc) >> 2];
	_output[1] = base64_chars[((_input[0] & 0x03) << 4) + ((_input[1] & 0xf0) >> 4)];
	_output[2] = base64_chars[((_input[1] & 0x0f) << 2) + ((_input[2] & 0xc0) >> 6)];
	_output[3] = base64_chars[  _input[2] & 0x3f];
}

void encode_block(char const* _input, char* _output, std::size_t len) {
	if (len == 0) {
		return;
	}
	auto input = std::array<char, 3>{'\0'};
	for (std::size_t i{0}; i < len; ++i) {
		input[i] = _input[i];
	}
	encode_block(&input[0], _output);

	if (len < 3) {
		_output[3] = '=';
	}
	if (len < 2) {
		_output[2] = '=';
	}
}

//!TODO must be unsigned to have correct shifting semantics
void decode_block(char const* _input, char* _output) {
	auto input = std::array<char, 4>{};
	for (std::size_t i{0}; i < input.size(); ++i) {
		input[i] = base64_chars.find(_input[i]);
	}
	_output[0] = ( input[0] << 2       ) + ((input[1] & 0x30) >> 4);
	_output[1] = ((input[1] & 0xf) << 4) + ((input[2] & 0x3c) >> 2);
	_output[2] = ((input[2] & 0x3) << 6) +   input[3];
}

void decode_block(char const* _input, char* _output, std::size_t len) {
	if (len == 0) {
		return;
	}
	auto input = std::array<char, 4>{'A'};
	input[0] = _input[0];
	input[1] = _input[1];
	if (len > 1) {
		input[2] = _input[2];
	}
	if (len > 2) {
		input[3] = _input[3];
	}

	auto output = std::array<char, 3>{};

	decode_block(&input[0], &output[0]);

	for (auto i{0}; i < len; ++i) {
		_output[i] = output[i];
	}
}

}

auto encoded_len(std::string_view _decoded) -> std::size_t {
	return 4 * ((_decoded.size() + 2) / 3);
}

auto encode(std::string_view _decoded) -> std::string {

	auto ret = std::string(encoded_len(_decoded), '=');
	for (int i{0}; i < _decoded.size() / 3; i += 1) {
		encode_block(&_decoded[i*3], &ret[i*4]);
	}
	auto rest_len = _decoded.size() % 3;
	int i = _decoded.size() / 3;
	encode_block(&_decoded[i * 3], &ret[i * 4], rest_len);
	return ret;
}

auto encoded_valid(std::string_view _encoded) -> bool {
	if (_encoded.empty()) {
		return true;
	}
	if (_encoded.size() % 4 != 0) {
		return false;
	}

	for (auto i{0}; i < _encoded.size()-2; ++i) {
		if (not is_base64(_encoded[i])) {
			return false;
		}
	}
	auto l1 = _encoded.size() - 1;
	auto l2 = _encoded.size() - 2;
	if (is_base64(_encoded[l2]) and is_base64(_encoded[l1])) {
		return true;
	}
	if (is_base64(_encoded[l2]) and _encoded[l1] == '=') {
		return true;
	}
	if (_encoded[l2] == '=' and _encoded[l1] == '=') {
		return true;
	}
	return false;
}

auto decoded_len(std::string_view _encoded) -> std::size_t {
	if (_encoded.size() == 0) {
		return 0;
	}

	auto size = _encoded.size();

	auto decoded_size = size * 3 / 4;
	if (_encoded[size-2] == '=') {
		decoded_size -= 2;
	} else if (_encoded[size-1] == '=') {
		decoded_size -= 1;
	}
	return decoded_size;
}

auto decode(std::string_view _encoded) -> std::string {
	if (_encoded.empty()) {
		return {};
	}

	auto ret = std::string(decoded_len(_encoded), '\0');

	for (auto i{0}; i<_encoded.size()/4; ++i) {
		decode_block(&_encoded[i*4], &ret[i*3]);
	}

	auto rest_len = ret.size() % 3;
	int i = _encoded.size()/4;
	decode_block(&_encoded[i*4], &ret[i*3], rest_len);
	return ret;
}

}
