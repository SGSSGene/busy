#pragma once

#include "Deserializer.h"

#include <fstream>
#include <sstream>
#include <queue>


#ifdef BUSY_GENERICFACTORY
	#include <genericFactory/genericFactory.h>
#endif


namespace serializer {

namespace binary {

class Serializer;

template<typename T>
class SerializerDefault {
private:
	Serializer& serializer;
	int32_t     id;
	T&          value;
	bool        needToKnowAddress;

public:
	SerializerDefault(Serializer& _serializer, int32_t _id, T& _value, bool _needToKnowAddress)
		: serializer ( _serializer  )
		, id         { _id }
		, value      ( _value )
		, needToKnowAddress { _needToKnowAddress }
	{
	}
	~SerializerDefault();
	void operator or(T const& t) {
	}
};

class SerializerNodeInput {
private:
	Serializer& serializer;
	int32_t     id;
	bool        needToKnowAddress;
public:
	SerializerNodeInput(Serializer& _serializer, int32_t _id, bool _needToKnowAddress);

	template<typename T>
	SerializerDefault<T> operator%(T& t);
};


class SerializerNode {
private:
	Serializer&      serializer;
	std::vector<int> index;
	int              startPoint;
	bool             needToKnowAddress;
public:
	SerializerNode(Serializer& _serializer, bool _needToKnowAddress);
	~SerializerNode();
	SerializerNodeInput operator[](std::string const& _str);
};

struct SerializerAdapter {
	Serializer& serializer;
	bool needToKnowAddress;

	SerializerAdapter(Serializer& _serializer, bool _needToKnowAddress)
		: serializer        ( _serializer )
		, needToKnowAddress { _needToKnowAddress }
	{}

	template<typename T>
	void serialize(T& _value);
	template<typename Iter>
	void serializeByIter(Iter iter, Iter end);
	template<typename Iter>
	void serializeMapByIter(Iter iter, Iter end);
	template<typename Iter>
	void serializeByIterCopy(Iter iter, Iter end);

};


class Serializer {
private:
	std::map<std::string, int32_t> stringToInt;

	std::vector<uint8_t> buffer;
	int                  currentPosition { 0 };

	int                  startPoint;
	SerializerNodeInput  rootNode {*this, -1, true};
	struct KnownAddress {
		void const* ptr;
		int32_t     count;
		int32_t     size;
		std::type_info const& type_info;
		int32_t     bufferPos; // Position where this address was serialized
	};
	std::vector<KnownAddress> knownAddresses;
	struct RawAddress {
		void const* ptr;       // Pointing to
		std::type_info const& type_info;
		int32_t     bufferPos; // position in serialized buffer
	};
	std::vector<RawAddress> rawAddresses;

	std::queue<std::function<void()>> sharedObjectFunctions;
	std::map<void const*, int32_t> sharedToId;

	bool mNoPointers;

public:
	Serializer(bool _noPointers = false) {
		mNoPointers = _noPointers;
		// serialize number which jumps directly to lookup table of string to int32
		serialize(int32_t(), false);

		startPoint = getCurrentPosition();
	}

	void close() {
		// Write string index to the end
		int endPoint = getCurrentPosition();
		int32_t size = endPoint - startPoint;
		memcpy(&buffer[startPoint - sizeof(int32_t)], &size, sizeof(size));

		int32_t posOfString = getCurrentPosition();
		serialize(int32_t(), false);
		// serialize shared objects
		int32_t posSharedObjectSize = getCurrentPosition();
		serialize(int32_t(), false);
		int32_t id = 0;
		while (not sharedObjectFunctions.empty()) {
			auto func = sharedObjectFunctions.front();
			sharedObjectFunctions.pop();
			serialize(id, false);
			serialize(int32_t(), false);
			auto sizePos = getCurrentPosition();
			func();
			int32_t size = getCurrentPosition() - sizePos;
			memcpy(&buffer[sizePos], &size, sizeof(size));
			++id;
		}
		memcpy(&buffer[posSharedObjectSize], &id, sizeof(int32_t));
		memcpy(&buffer[posOfString], &currentPosition, sizeof(int32_t));
		// serialize string to int
		int32_t stringSize = int32_t(stringToInt.size());
		serialize(stringSize, false);
		for (auto const& e : stringToInt) {
			serialize(e.second, false);
			serialize(e.first, false);
		}

		// Fill all rawpointers
		for (auto const& raw : rawAddresses) {
			for (auto const& known : knownAddresses) {
				if (raw.type_info.hash_code() != known.type_info.hash_code()) continue;
				if (raw.ptr < known.ptr) continue;
				if (raw.ptr >= (uint8_t*)known.ptr + (known.size*known.count)) continue;

				int32_t offset = 0;
				for (; offset < known.count*known.size; offset += known.size) {
					if ((uint8_t*)known.ptr + offset == raw.ptr) {
						break;
					}
				}
				// !TODO offset should be saved in a second field
				int32_t value = known.bufferPos + offset;
				memcpy(&buffer[raw.bufferPos], &value, sizeof(value));

				break;
			}
		}




	}

	SerializerNodeInput const& getRootNode() const {
		return rootNode;
	}
	SerializerNodeInput& getRootNode() {
		return rootNode;
	}


	uint8_t* getPtr() {
		return &buffer[0];
	}

	auto getData() const -> std::vector<uint8_t> const& {
		return buffer;
	}
	auto releaseData() -> std::vector<uint8_t> {
		return std::move(buffer);
	}


	std::string getDataAsStr() const {
		return "<binary>";
	}

	void addKnownAddress(void const* _ptr, int32_t  _count, int32_t _size, int32_t _bufferPos, std::type_info const& _type_info) {
		if (not mNoPointers) {
			knownAddresses.push_back({_ptr, _count, _size, _type_info, _bufferPos});
		}
	}

	template<typename T>
	int32_t addSharedObject(std::shared_ptr<T>& _value) {
		if (mNoPointers) {
			throw std::runtime_error("Serialize: serialization failed, rcan't add shared pointers in a no pointer environment.");
		}
		if (sharedToId.find(_value.get()) == sharedToId.end()) {
			int32_t id = sharedToId.size();
			sharedToId[_value.get()] = id;
			sharedObjectFunctions.push([=]() {

				// Extrem hacky!!! casting shared object to unique to use it's serialization
				std::unique_ptr<T> ptr (_value.get());
				serialize(ptr, true);
				ptr.release();

			});
		}
		return sharedToId.at(_value.get());
	}



	int getCurrentPosition() const {
		return currentPosition;
	}
	void setCurrentPosition(int _pos) {
		currentPosition = _pos;
	}


	void serialize(void const* _data, int32_t _count, int32_t _size, bool _needToKnowAddress, std::type_info const& _type_info) {
		if (int(buffer.size()) < currentPosition + _count * _size) {
			buffer.resize(currentPosition + _count * _size);
		}
		memcpy(&buffer[currentPosition], _data, _count * _size);
		if (_needToKnowAddress) {
			addKnownAddress(_data, _count, _size, getCurrentPosition(), _type_info);
		}
		currentPosition += _count * _size;
	}

	int32_t mapStringToInt(std::string const& _str) {
		if (stringToInt.find(_str) == stringToInt.end()) {
			int32_t size = int32_t(stringToInt.size());
			stringToInt[_str] = size;
		}

		return stringToInt.at(_str);
	}

	void serialize(std::string const& _str, bool _needToKnowAddress) {
		if (_needToKnowAddress) {
			addKnownAddress(&_str, 1, sizeof(_str), getCurrentPosition(), typeid(std::string));
		}
		int32_t size = _str.length();
		serialize(size, false);
		serialize(_str.c_str(), size, sizeof(char), false, typeid(char));
	}

	template<typename T, typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
	void serialize(T const& _value, bool _needToKnowAddress) {

		serialize(&_value, 1, sizeof(T), _needToKnowAddress, typeid(T));
	}

	template<typename T, int N, typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
	void serialize(std::array<T, N> const& _value, bool _needToKnowAddress) {
		if (_needToKnowAddress) {
			addKnownAddress(&_value, 1, sizeof(std::vector<T>), getCurrentPosition(), typeid(std::array<T, N>));
		}

		serialize(_value.data(), N, sizeof(T), _needToKnowAddress, typeid(T));
	}

	template<typename T, typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
	void serialize(std::vector<T>& _value, bool _needToKnowAddress) {

		if (_needToKnowAddress) {
			addKnownAddress(&_value, 1, sizeof(std::vector<T>), getCurrentPosition(), typeid(std::vector<T>));
		}
		int32_t size = _value.size();
		serialize(size, false);
		serialize(_value.data(), size, sizeof(T), _needToKnowAddress, typeid(T));
	}

	template<typename T, typename std::enable_if<has_serialize_function<T, SerializerNode>::value>::type* = nullptr>
	void serialize(T& _value, bool _needToKnowAddress) {
		if (_needToKnowAddress) {
			addKnownAddress(&_value, 1, sizeof(T), getCurrentPosition(), typeid(T));
		}
		SerializerNode node(*this, _needToKnowAddress);

#ifndef BUSY_GENERICFACTORY
		_value.serialize(node);
#else
		if (genericFactory::hasType(&_value)) {
			genericFactory::serialize(&_value, node);
		} else {
			_value.serialize(node);
		}
#endif

	}
	template<typename T>
	void serialize(T*& _value, bool _needToKnowAddress) {
		if (_needToKnowAddress) {
			addKnownAddress(&_value, 1, sizeof(T*), getCurrentPosition(), typeid(T*));
		}

		if (_value != nullptr) {
			int32_t pos = getCurrentPosition();
			rawAddresses.push_back(RawAddress{_value, typeid(T), pos});
		}
		serialize(int32_t(), false);
	}
	template<typename T>
	void serialize(std::shared_ptr<T>& _value, bool _needToKnowAddress) {
		if (_needToKnowAddress) {
			addKnownAddress(&_value, 1, sizeof(std::shared_ptr<T>), getCurrentPosition(), typeid(std::shared_ptr<T>));
		}

		int32_t ptrId = {-1};
		if (_value.get() != nullptr) {
			ptrId = addSharedObject(_value);
		}
		serialize(ptrId, false);

	}

	template<typename T, typename std::enable_if<not std::is_fundamental<T>::value
	                                             and not has_serialize_function<T, SerializerNode>::value>::type* = nullptr>
	void serialize(T& _value, bool _needToKnowAddress) {
		if (_needToKnowAddress) {
			addKnownAddress(&_value, 1, sizeof(T), getCurrentPosition(), typeid(T));
		}

		SerializerAdapter adapter(*this, _needToKnowAddress);
		Converter<T>::serialize(adapter, _value);

	}
};

template<typename T>
SerializerDefault<T>::~SerializerDefault() {
	// Serializing root object
	if (id == -1) {
		serializer.serialize(value, needToKnowAddress);
		return;
	}

	serializer.serialize(id, false);
	serializer.serialize(int32_t(), false);
	int32_t sizePos = serializer.getCurrentPosition();

	serializer.serialize(value, needToKnowAddress);

	int32_t endPos = serializer.getCurrentPosition();
	int32_t size = endPos - sizePos;
	memcpy(serializer.getPtr() + sizePos-sizeof(int32_t), &size, sizeof(size));
}

template<typename T>
SerializerDefault<T> SerializerNodeInput::operator%(T& t) {
	return SerializerDefault<T>(serializer, id, t, needToKnowAddress);
}

template<typename T>
void SerializerAdapter::serialize(T& _value) {
	serializer.serialize(_value, needToKnowAddress);
}

template<typename Iter>
void SerializerAdapter::serializeByIter(Iter iter, Iter end) {
	int32_t size = std::distance(iter, end);
	serialize(size);
	for (;iter != end; ++iter) {
		serialize(*iter);
	}
}
template<typename Iter>
void SerializerAdapter::serializeMapByIter(Iter iter, Iter end) {
	int32_t size = std::distance(iter, end);
	serialize(size);
	for (;iter != end; ++iter) {
		serialize(*iter);
	}
}


template<typename Iter>
void SerializerAdapter::serializeByIterCopy(Iter iter, Iter end) {
	int32_t size = std::distance(iter, end);
	serialize(size);
	for (;iter != end; ++iter) {
		auto t = *iter;
		serialize(t);
	}
}

template <typename T>
void read(std::vector<uint8_t> _data, T& _value, bool usenopointers=false) {
	// parse file in serializer
	Deserializer serializer(std::move(_data), usenopointers);
	serializer.getRootNode() % _value;
	serializer.close();
}


template <typename T>
void read(std::string const& _file, T& _value, bool usenopointers=false) {

	// Read file from storage
	std::ifstream ifs(_file, std::ios::binary);
	if (ifs.fail()) {
		throw std::runtime_error("Opening file failed");
	}

	ifs.seekg(0, std::ios::end);
	std::vector<uint8_t> data(ifs.tellg());
	ifs.seekg(0, std::ios::beg);
	ifs.read((char*)&data[0], int(data.size()));
	ifs.close();

	read(std::move(data), _value, usenopointers);
}

template <typename T>
auto write(T const& _value, bool usenopointers = false) -> std::vector<uint8_t> {
	// Serialize data
	Serializer serializer(usenopointers);
	serializer.getRootNode() % const_cast<T&>(_value); // Ugly const cast, but we know that we are only reading
	serializer.close();
	return serializer.releaseData();
}
template<typename T>
void write(std::string const& _file, T const& _value, bool usenopointers = false) {
	std::ofstream oFile(_file, std::ios::binary);
	if (oFile.fail()) {
		throw std::runtime_error("Opening file failed");
	}
	auto data = write(_value, usenopointers);
	oFile.write((char const*)&data[0], int(data.size()));
	oFile.close();
	if (oFile.fail()) {
		throw std::runtime_error("Writing to file failed");
	}
}



}
}
