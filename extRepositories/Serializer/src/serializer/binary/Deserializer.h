#pragma once

#include <serializer/Converter.h>
#include <serializer/has_serialize_function.h>

#include <cassert>
#include <cstring>
#include <functional>
#include <map>
#include <memory>
#include <type_traits>
#include <vector>

#ifdef BUSY_GENERICFACTORY
	#include <genericFactory/genericFactory.h>
#endif


namespace serializer {
namespace binary {

class Deserializer;

template<typename T>
class DeserializerDefault {
private:
	Deserializer& serializer;
	T&            value;
	bool          available;
	bool          defaultValue;
	bool          needToKnowAddress;

public:
	DeserializerDefault(Deserializer& _serializer, T& _value, bool _available, bool _needToKnowAddress)
		: serializer ( _serializer )
		, value      ( _value )
		, available  { _available }
		, defaultValue { false }
		, needToKnowAddress { _needToKnowAddress }
	{ }

	~DeserializerDefault();

	template<typename T2, typename std::enable_if<std::is_default_constructible<T2>::value
	                                              and std::is_assignable<T2, T2>::value>::type* = nullptr>
	void getDefault(T2& _value) const {
		_value = T2();
	}
	template<typename T2, typename std::enable_if<not std::is_default_constructible<T2>::value
	                                              or not std::is_assignable<T2, T2>::value>::type* = nullptr>
	void getDefault(T2& _value) const {
		(void)_value;
		throw std::runtime_error("trying to construct object without default constructor");
	}


	template<typename T2, typename std::enable_if<std::is_assignable<T2, T2>::value or std::is_pod<T2>::value>::type* = nullptr>
	void operator or(T2 const& t) {
		if (not available) {
			defaultValue = true;
			value = t;
		}
	}

	template<typename T2, typename std::enable_if<not std::is_assignable<T2, T2>::value and not std::is_pod<T2>::value>::type* = nullptr>
	void operator or(T2 const& t) {
		(void)t;
		throw std::runtime_error(std::string("trying to us \"or\" on a not copyable datatype: ") + typeid(T2).name());
	}

};


class DeserializerNodeInput {
private:
	Deserializer& serializer;
	bool          available;
	bool          needToKnowAddress;
public: DeserializerNodeInput(Deserializer& _serializer, bool _available, bool _needToKnowAddress)
		: serializer ( _serializer )
		, available  { _available}
		, needToKnowAddress { _needToKnowAddress }
	{
	}

	template<typename T>
	DeserializerDefault<T> operator%(T& t);
};

class DeserializerNode {
private:
	Deserializer&    serializer;
	std::vector<int> index;
	int32_t          startPoint;
	int32_t          size;
	bool             needToKnowAddress;
public:
	DeserializerNode(Deserializer& _serializer, bool _available, bool _needToKnowAddress);
	~DeserializerNode();
	DeserializerNodeInput operator[](std::string const& _str);
};

struct DeserializerAdapter {
	Deserializer& serializer;
	bool          needToKnowAddress;
	DeserializerAdapter(Deserializer& _serializer, bool _needToKnowAddress)
		: serializer        ( _serializer )
		, needToKnowAddress { _needToKnowAddress }
	{}

	template<typename T>
	void deserialize(T& _value);
	template<typename T>
	void deserializeByInsert(std::function<void(T& v)> _func);
	template<typename Key, typename Value>
	void deserializeMap(std::map<Key, Value>& _map);
};


class Deserializer {
private:
	std::map<std::string, int32_t> stringToInt;
	std::map<int32_t, std::string> intToString;

	std::vector<uint8_t> buffer;
	int                  currentPosition { 0 };

	bool useNoPointers;

	DeserializerNodeInput rootNode { *this, true, true};

	struct KnownAddress {
		void const* ptr;
		int32_t     count;
		int32_t     size;
		std::type_info const& type_info;
		int32_t     bufferPos; // Position where this address was serialized
	};
	std::vector<KnownAddress> knownAddresses;
	struct RawAddress {
		void*       ptr;       // Pointing to
		std::type_info const& type_info;
		int32_t     bufferPos; // position in serialize buffer
	};
	std::vector<RawAddress> rawAddresses;

	std::map<int32_t, int32_t> ptrIdToBufferPos;
	std::map<int32_t, std::shared_ptr<void>> idToShared;

public:
	Deserializer(std::vector<uint8_t> _data, bool usenopointers = false)
		: buffer {std::move(_data)}
		, useNoPointers {usenopointers}
	{

		intToString[-1] = "";// Special entry that indicates not found string

		int32_t size;
		deserialize(size, false);
		int stack = getCurrentPosition();
		setCurrentPosition(sizeof(int32_t) + size);


		int32_t stringPos;
		deserialize(stringPos, false);

		int32_t sharedPos = getCurrentPosition();

		setCurrentPosition(stringPos);

		// Deseiralizing int to string (of string compression)
		int32_t stringSize;
		deserialize(stringSize, false);

		for (int i {0} ; i < stringSize; ++i) {
			int32_t index;
			std::string str;
			deserialize(index, false);
			deserialize(str, false);

			stringToInt[str]   = index;
			intToString[index] = str;
		}

		setCurrentPosition(sharedPos);
		// Reading shared object positions
		// serialize shared objects
		int32_t sharedObjectSize;

		deserialize(sharedObjectSize, false);
		for (int32_t i {0}; i < sharedObjectSize; ++i) {
			int32_t ptrId;
			deserialize(ptrId, false);
			int32_t sizePos;
			deserialize(sizePos, false);
			ptrIdToBufferPos[ptrId] = getCurrentPosition();

			setCurrentPosition(getCurrentPosition() + sizePos);
		}

		setCurrentPosition(stack);
	}
	void close() {
		// Fill all rawpointers
		for (auto const& raw : rawAddresses) {
			for (auto const& known : knownAddresses) {
				if (raw.type_info.hash_code() != known.type_info.hash_code()) continue;
				if (raw.bufferPos >= known.bufferPos and raw.bufferPos < known.bufferPos + known.size*known.count) {
					int32_t offset = 0;
					for (; offset < known.count*known.size; offset += known.size) {
						if (known.bufferPos + offset == raw.bufferPos) {
							break;
						}
					}
					void* value = (uint8_t*)known.ptr + offset;
					memcpy(raw.ptr, &value, sizeof(void*));
					break;
				}
			}
		}
	}

	DeserializerNodeInput const& getRootNode() const {
		return rootNode;
	}
	DeserializerNodeInput& getRootNode() {
		return rootNode;
	}

	void addKnownAddress(void const* _ptr, int32_t  _count, int32_t _size, int32_t _bufferPos, std::type_info const& _type_info) {
		if (not useNoPointers) {
			knownAddresses.push_back({_ptr, _count, _size, _type_info, _bufferPos});
		}

	}

	template<typename T>
	void getSharedObject(int32_t _ptrId, std::shared_ptr<T>& _value) {
		if (idToShared.find(_ptrId) == idToShared.end()) {
			auto currentPos = getCurrentPosition();

			setCurrentPosition(ptrIdToBufferPos.at(_ptrId));
			std::unique_ptr<T> value;
			deserialize(value, true);
			idToShared[_ptrId] = std::shared_ptr<T>(value.release());

			setCurrentPosition(currentPos);
		}
		_value = std::static_pointer_cast<T, void>(idToShared.at(_ptrId));
	}


	uint8_t* getPtr() {
		return &buffer[0];
	}

	int getCurrentPosition() const {
		return currentPosition;
	}
	void setCurrentPosition(int _pos) {
		currentPosition = _pos;
	}

	void deserialize(void* _data, int32_t _count, int32_t _size, bool _needToKnowAddress, std::type_info const& _type_info) {
		if (int(buffer.size()) < currentPosition + _count*_size) {
			throw std::runtime_error("runtime error! (reason is secret)");
		}

		memcpy(_data, &buffer[currentPosition], _count*_size);
		if (_needToKnowAddress) {
			addKnownAddress(_data, _count, _size, getCurrentPosition(), _type_info);
		}


		currentPosition += _count * _size;
	}

	std::string const& mapIntToString(int32_t _index) const {
		if (intToString.find(_index) != intToString.end()) {
			return intToString.at(_index);
		}
		return intToString.at(-1);
	}

	void deserialize(std::string& _str, bool _needToKnowAddress) {
		if (_needToKnowAddress) {
			addKnownAddress(&_str, 1, sizeof(std::string), getCurrentPosition(), typeid(std::string));
		}

		int32_t size;
		deserialize(size, false);
		std::vector<char> tBuffer(size+1, '\0');
		deserialize(&tBuffer[0], size, sizeof(char), false, typeid(char));
		_str = &tBuffer[0];
	}

	template<typename T, typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
	void deserialize(T& _value, bool _needToKnowAddress) {
		deserialize(&_value, 1, sizeof(T), _needToKnowAddress, typeid(T));
	}

	template<typename T, int N, typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
	void deserialize(std::array<T, N>& _value, bool _needToKnowAddress) {
		if (_needToKnowAddress) {
			addKnownAddress(&_value, N, sizeof(T), getCurrentPosition(), typeid(std::array<T, N>));
		}
		deserialize(_value.data(), N, sizeof(T), _needToKnowAddress, typeid(T));
	}

	template<typename T, typename std::enable_if<std::is_fundamental<T>::value>::type* = nullptr>
	void deserialize(std::vector<T>& _value, bool _needToKnowAddress) {
		int32_t size;
		if (_needToKnowAddress) {
			addKnownAddress(&_value, 1, sizeof(T), getCurrentPosition(), typeid(std::vector<T>));
		}
		deserialize(size, false);
		if (_value.size() != size) {
			_value = std::vector<T>(size);
		}
		deserialize(_value.data(), size, sizeof(T), _needToKnowAddress, typeid(T));
	}

	template<typename T, typename std::enable_if<(has_serialize_function<T, DeserializerNode>::value)>::type* = nullptr>
	void deserialize(T& _value, bool _needToKnowAddress) {
		if (_needToKnowAddress) {
			addKnownAddress(&_value, 1, sizeof(T), getCurrentPosition(), typeid(T));
		}
		DeserializerNode node(*this, true, _needToKnowAddress);
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
	void deserialize(T*& _value, bool _needToKnowAddress) {
		_value = nullptr;
		if (_needToKnowAddress) {
			addKnownAddress(&_value, 1, sizeof(T*), getCurrentPosition(), typeid(T*));
		}

		int32_t pos;
		deserialize(pos, false);
		if (pos != 0) {
			rawAddresses.push_back(RawAddress{&_value, typeid(T), pos});
		}
	}

	template<typename T>
	void deserialize(std::shared_ptr<T>& _value, bool _needToKnowAddress) {
		if (_needToKnowAddress) {
			addKnownAddress(&_value, 1, sizeof(std::shared_ptr<T>), getCurrentPosition(), typeid(std::shared_ptr<T>));
		}

		int32_t ptrId;
		deserialize(ptrId, false);
		if (ptrId != -1) {
			getSharedObject(ptrId, _value);
		} else {
			_value.reset();
		}

	}


	template<typename T, typename std::enable_if<not std::is_fundamental<T>::value
	                                             and not has_serialize_function<T, DeserializerNode>::value>::type* = nullptr>
	void deserialize(T& _value, bool _needToKnowAddress) {
		if (_needToKnowAddress) {
			addKnownAddress(&_value, 1, sizeof(T), getCurrentPosition(), typeid(T));
		}

		DeserializerAdapter adapter(*this, _needToKnowAddress);
		Converter<T>::deserialize(adapter, _value);
	}
};

template<typename T>
DeserializerDefault<T>::~DeserializerDefault() {
	if (not available and not defaultValue) {
		getDefault<T>(value);
	} else if (available) {
		serializer.deserialize(value, needToKnowAddress);
	}
}

template<typename T>
DeserializerDefault<T> DeserializerNodeInput::operator%(T& t) {
	return DeserializerDefault<T>(serializer, t, available, needToKnowAddress);
}

template<typename T>
void DeserializerAdapter::deserialize(T& _value) {
	serializer.deserialize(_value, needToKnowAddress);
}

template<typename T>
void DeserializerAdapter::deserializeByInsert(std::function<void(T& v)> _func) {
	int32_t size;
	serializer.deserialize(size, needToKnowAddress);
	for (int i {0}; i < size; ++i) {
		T value;
		serializer.deserialize(value, true);
		_func(value);
	}
}
template<typename Key, typename Value>
void DeserializerAdapter::deserializeMap(std::map<Key, Value>& _map) {
	int32_t size;
	serializer.deserialize(size, needToKnowAddress);
	for (int i {0}; i < size; ++i) {
		std::pair<Key, Value> value;
		serializer.deserialize(value, true);
		_map[value.first] = value.second;
	}

}



}
}
