#pragma once

#include "utils.h"

#include <cstring>
#include <exception>
#include <optional>
#include <tuple>

namespace fon::binary::details {
namespace serializer {

template <typename T>
struct Pointer {
	std::vector<std::byte>& buffer;
	size_t index;

	Pointer(std::vector<std::byte>& _buffer)
		: buffer{_buffer}
		, index{buffer.size()}
	{
		buffer.resize(index + sizeof(T));
	}
	auto operator=(T const& t) -> Pointer& {
		std::memcpy(&buffer[index], &t, sizeof(T));
		return *this;
	}
	void reset() {
		buffer.resize(index);
	}
	auto size() const {
		return buffer.size() - index;
	}
};

struct Frame {
	std::vector<std::byte>& buffer;
	size_t index;
	std::optional<Pointer<size_t>>  sizePtr;
	std::optional<Pointer<uint8_t>> typePtr;
	enum class Type : uint8_t {None = 0x00,  Value = 0x01, Sequence = 0x02, Map = 0x03};
	Type mType {Type::None};

	size_t entries{0};
	std::optional<Pointer<size_t>> entriesPtr;

	Frame(std::vector<std::byte>& _buffer)
		: buffer{_buffer}
		, index {buffer.size()}
	{}

	~Frame() {
		//!TODO roll back doesn't seem to work, some unit test are failing
		#if 0
		// roll back if no data was written and nothing has to be written
		if (buffer.size() == index + sizeof(mType) + sizeof(size_t)
			and (not entriesPtr or entries == 0)) {
			std::cout << "roll back: entries: " << entries << "\n";
			buffer.resize(index);
			return;
		}
		#endif

		// finalize different data
		if (sizePtr) {
			(*sizePtr) = sizePtr->size();
		}
		if (typePtr) {
			(*typePtr) = uint8_t(mType);
		}

		if (entriesPtr) {
			(*entriesPtr) = entries;
		}
	}
	void setType(Type type) {
		if (mType == Type::None) {
			sizePtr.emplace(buffer);
			typePtr.emplace(buffer);
			if (type == Type::Sequence or type == Type::Map) {
				entriesPtr.emplace(buffer);
			}
		} else if (mType != type) {
			throw std::runtime_error("type can't be changed");
		}
		mType = type;
	}

	template <typename T>
	auto operator=(T const& obj) -> Frame& {
		setType(Type::Value);
		Pointer<T>{buffer} = obj;
		return *this;
	}

	template <typename T>
	void set(T const* obj, size_t size) {
		setType(Type::Value);
		buffer.resize(buffer.size() + size);
		std::memcpy(buffer.data() + buffer.size() - size, obj, size);
	}

	void push_back() {
		setType(Type::Sequence);
		entries += 1;
	}

	void push_back(std::vector<std::byte> key) {
		setType(Type::Map);

		buffer.resize(buffer.size() + key.size());
		std::memcpy(buffer.data() + buffer.size() - key.size(), key.data(), key.size());
		entries += 1;
	}


};
}

using SerializeStack = serializer::Frame;

}
