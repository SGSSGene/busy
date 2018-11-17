#pragma once

#include "utils.h"

#include <cstring>
#include <exception>
#include <optional>
#include <tuple>

namespace fon::binary::details {

namespace deserializer {

struct Frame {
	std::vector<std::byte>& buffer;
	size_t start;

	size_t size;

	enum class Type : uint8_t {None = 0x00,  Value = 0x01, Sequence = 0x02, Map = 0x03};
	Type type;

	size_t entries{0};

	size_t nextIndexEntries;

	Frame(std::vector<std::byte>& _buffer, size_t index)
		: buffer{_buffer}
		, start{index}
	{
		if (start + sizeof(size) + 1 > buffer.size()) {
			throw std::runtime_error("frame to small");
		}
		std::memcpy(&size, buffer.data() + start, sizeof(size));
		std::memcpy(&type, buffer.data() + start + sizeof(size), sizeof(type));

		if (type == Type::Sequence or type == Type::Map) {
			std::memcpy(&entries, buffer.data() + start + sizeof(size) + sizeof(type), sizeof(entries));
			nextIndexEntries = start + sizeof(size) + sizeof(type) + sizeof(entries);
		}

		if (start + size > buffer.size()) {
			throw std::runtime_error("error initializing new binary stack buffer, file corrupted?");
		}
	}
	~Frame() {}

	auto getType() const {
		return type;
	}

	template <typename T>
	auto as() const -> T {
		if (type != Type::Value) {
			throw std::runtime_error("type changed");
		}
		if (size != sizeof(size) + sizeof(T) + sizeof(type)) {
			throw std::runtime_error("unexpected size");
		}

		T t{};
		std::memcpy(&t, data<void>(), sizeof(T));
		return t;
	}
	template <typename T>
	auto data() const -> T const* {
		if (type != Type::Value) {
			throw std::runtime_error("type changed");
		}
		return reinterpret_cast<T const*>(buffer.data() + start + sizeof(size) + sizeof(type));
	}
	auto payloadSize() const {
		return size - sizeof(type) - sizeof(size);
	}

	auto end() const {
		return start + size;
	}

	auto nextIndex() {
		auto next = nextIndexEntries;
		auto frame = Frame{buffer, nextIndexEntries};
		nextIndexEntries += frame.size;
		return next;

	}
};

}

using DeserializeStack = deserializer::Frame;

}
