#pragma once

#include <cstring>
#include <functional>
#include <yaml-cpp/yaml.h>
#include <limits>
#include <memory>

#include "../Converter.h"
#include "../has_serialize_function.h"

#ifdef BUSY_GENERICFACTORY
	#include <genericFactory/genericFactory.h>
#endif


namespace serializer {
namespace yaml {

struct StringConvertFrom {
	static std::string from(std::string const& _value) {
		return _value;
	}
	template <typename T, typename std::enable_if<not std::is_enum<T>::value
	                                             && not std::is_same<T, uint8_t>::value
	                                             && not std::is_same<T, int8_t>::value>::type* = nullptr>
	static T from(std::string const& _value) {
		T typedKey;
		std::stringstream ss;
		ss << _value;
		ss >> typedKey;
		return typedKey;
	}
	template <typename T, typename std::enable_if<not std::is_enum<T>::value
	                                             && (std::is_same<T, uint8_t>::value
	                                             ||  std::is_same<T, int8_t>::value)>::type* = nullptr>
	static T from(std::string const& _value) {
		return T(from<int>(_value));
	}


	template <typename T, typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
	static T from(std::string const& _value) {
		using Type = typename std::underlying_type<T>::type;
		return T(from<Type>(_value));
	}
};
template<typename T>
T from_string(std::string const& _value) {
	return StringConvertFrom::from<T>(_value);
}

using NodePath = std::vector<std::string>;

inline auto to_string(NodePath const& _path) -> std::string {
	if(_path.empty()) {
		return "";
	}
	std::string retValue = _path[0];
	for (size_t i(1); i < _path.size(); ++i) {
		retValue += "." + _path[i];
	}
	return retValue;
}

class Deserializer;

template<typename T>
class DeserializerDefault {
private:
	Deserializer& serializer;
	T&            value;
	YAML::Node&   node;
	bool          available;
	bool          defaultValue;
	NodePath      nodePath;

public:
	DeserializerDefault(Deserializer& _serializer, T& _value, YAML::Node& _node, bool _available, NodePath const& _nodePath)
		: serializer ( _serializer )
		, value      ( _value )
		, node       ( _node )
		, available  { _available }
		, defaultValue { false }
		, nodePath   { _nodePath }
	{ }
	~DeserializerDefault();

	template<typename T2, typename std::enable_if<std::is_default_constructible<T2>::value
	                                              and std::is_assignable<T2&, T2>::value>::type* = nullptr>
	void getDefault(T2& _value) const {
		_value = T2();
	}
	template<typename T2, typename std::enable_if<not std::is_default_constructible<T2>::value
	                                              or not std::is_assignable<T2&, T2>::value>::type* = nullptr>
	void getDefault(T2&) const {
		throw std::runtime_error("trying to construct object without default constructor");
	}

	template<typename T2, typename std::enable_if<std::is_assignable<T2&, T2>::value>::type* = nullptr>
	void operator or(T2 const& t) {
		if (not available) {
			defaultValue = true;
			value = t;
		}
	}
	template<typename T2, typename std::enable_if<not std::is_assignable<T2&, T2>::value>::type* = nullptr>
	void operator or(T2 const& t) {
		throw std::runtime_error("trying to us \"or\" on a not copyable datatype");
	}

};


class DeserializerNodeInput {
private:
	Deserializer& serializer;
	YAML::Node&   node;
	bool          available;
	NodePath      nodePath;
public:
	DeserializerNodeInput(Deserializer& _serializer, YAML::Node& _node, bool _available, NodePath const& _nodePath)
		: serializer ( _serializer )
		, node       ( _node )
		, available  { _available}
		, nodePath   { _nodePath }
	{
	}

	template<typename T>
	DeserializerDefault<T> operator%(T& t);
};

class DeserializerNode {
private:
	Deserializer&    serializer;
	std::vector<int> index;
	YAML::Node&      node;
	NodePath         nodePath;

	std::list<YAML::Node> _nodes;

	std::set<std::string> mAccessed;

public:
	DeserializerNode(Deserializer& _serializer, YAML::Node& _node, bool _available, NodePath const& _nodePath);
	~DeserializerNode();
	auto operator[](std::string const& _str) -> DeserializerNodeInput;
};

struct DeserializerAdapter {
	Deserializer& serializer;
	YAML::Node&   node;
	NodePath      nodePath;
	DeserializerAdapter(Deserializer& _serializer, YAML::Node& _node, NodePath const& _nodePath)
		: serializer ( _serializer )
		, node       ( _node )
		, nodePath   { _nodePath }
	{}

	template<typename T>
	void deserialize(T& _value);
	template<typename T>
	void deserializeByInsert(std::function<void(T& v)> _func);
	template<typename Key, typename Value>
	void deserializeMap(std::map<Key, Value>& _map);
};


class Deserializer {
	YAML::Node node;

	DeserializerNodeInput rootNode {*this, node, true, {}};

	struct KnownAddress {
		void const* ptr;
		NodePath    bufferPos; // Position where this address was serialized
	};
	std::map<NodePath, KnownAddress> knownAddresses;
	struct RawAddress {
		std::vector<void*> needOverwrite;
	};
	std::map<int32_t, RawAddress> rawAddresses;

	YAML::Node sharedObjectNode;
	std::map<int32_t, std::shared_ptr<void>> idToShared;

	std::map<std::string, YAML::Node> mUnusedFields;
public:
	Deserializer(std::vector<uint8_t> const& _data);
	Deserializer(std::string const& _data);

	void close();

	DeserializerNodeInput const& getRootNode() const {
		return rootNode;
	}
	DeserializerNodeInput& getRootNode() {
		return rootNode;
	}

	void addKnownAddress(void const* _ptr, NodePath const& _bufferPos) {
		knownAddresses[_bufferPos] = {_ptr, _bufferPos};
	}

	template<typename T>
	void getSharedObject(int32_t _ptrId, std::shared_ptr<T>& _value) {
		if (idToShared.find(_ptrId) == idToShared.end()) {
			// Extrem hacky!!! casting shared object to unique to use it's serialization
			std::unique_ptr<T> value;
			auto tnode = sharedObjectNode[_ptrId];
			deserialize(tnode, value, {"__sharedObjects"});
			idToShared[_ptrId] = std::shared_ptr<T>(value.release());
		}
		_value = std::static_pointer_cast<T, void>(idToShared.at(_ptrId));
	}


	YAML::Node& getNode() { return node; }

	template<typename T, typename std::enable_if<std::is_same<T, bool>::value
	                                             or std::is_same<T, uint16_t>::value
	                                             or std::is_same<T, int16_t>::value
	                                             or std::is_same<T, uint16_t>::value
	                                             or std::is_same<T, int32_t>::value
	                                             or std::is_same<T, uint32_t>::value
	                                             or std::is_same<T, int64_t>::value
	                                             or std::is_same<T, uint64_t>::value
	                                             or std::is_same<T, float>::value
	                                             or std::is_same<T, double>::value
	                                             or std::is_same<T, std::string>::value>::type* = nullptr>
	void deserialize(YAML::Node& _node, T& _value, NodePath const& _nodePath) {
		_value = _node.as<T>();
		addKnownAddress(&_value, _nodePath);
	}

	template<typename T, typename std::enable_if<std::is_same<T, uint8_t>::value
	                                             or std::is_same<T, int8_t>::value>::type* = nullptr>
	void deserialize(YAML::Node& _node, T& _value, NodePath const& _nodePath) {
		_value = _node.as<int16_t>();
		addKnownAddress(&_value, _nodePath);
	}


	template<typename T, typename std::enable_if<has_serialize_function<T, DeserializerNode>::value>::type* = nullptr>
	void deserialize(YAML::Node& _node, T& _value, NodePath const& _nodePath) {
		addKnownAddress(&_value, _nodePath);

		DeserializerNode node(*this, _node, true, _nodePath);
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
	void deserialize(YAML::Node& _node, T*& _value, NodePath const& _nodePath) {
		int32_t ptrId;
		deserialize(_node, ptrId, _nodePath);
		_value = nullptr;
		rawAddresses[ptrId].needOverwrite.push_back((void*)&_value);
		addKnownAddress(&_value, _nodePath);

	}

	template<typename T>
	void deserialize(YAML::Node& _node, std::shared_ptr<T>& _value, NodePath const& _nodePath) {
		_value.reset();

		int32_t ptrId;
		deserialize(_node, ptrId, _nodePath);
		if (ptrId >= 0) {
			getSharedObject(ptrId, _value);
		}
	}

	template<typename T, typename std::enable_if<not std::is_fundamental<T>::value
	                                             and not std::is_same<T, std::string>::value
	                                             and not has_serialize_function<T, DeserializerNode>::value>::type* = nullptr>
	void deserialize(YAML::Node& _node, T& _value, NodePath const& _nodePath) {
		addKnownAddress(&_value, _nodePath);

		DeserializerAdapter adapter(*this, _node, _nodePath);
		Converter<T>::deserialize(adapter, _value);

	}

	void addUnusedFields(std::string const& _path, YAML::Node const& _node) {
		mUnusedFields[_path] = _node;
	}
	auto getUnusedFields() const -> std::map<std::string, YAML::Node> const& {
		return mUnusedFields;
	}

};

template<typename T>
DeserializerDefault<T>::~DeserializerDefault() {
	if (not available and not defaultValue) {
		getDefault<T>(value);
	} else if (available) {
		auto tnode = node;
		serializer.deserialize(tnode, value, nodePath);
	}
}

template<typename T>
DeserializerDefault<T> DeserializerNodeInput::operator%(T& t) {
	return DeserializerDefault<T>(serializer, t, node, available, nodePath);
}

template<typename T>
void DeserializerAdapter::deserialize(T& _value) {
	serializer.deserialize(node, _value, nodePath);
}

template<typename T>
void DeserializerAdapter::deserializeByInsert(std::function<void(T& v)> _func) {
	//if (not node.IsSequence() and not node.IsNull()) throw std::runtime_error("expected array");
	if (node.IsSequence()) {
		int32_t index { 0 };
		for (auto v : node) {
			T value;
			NodePath newNodePath = nodePath;
			newNodePath.emplace_back(std::to_string(index++));
			serializer.deserialize(v, value, newNodePath);
			_func(value);
		}
	}
}

template<typename Key, typename Value>
void DeserializerAdapter::deserializeMap(std::map<Key, Value>& _map) {
	if (node.IsMap()) {
		for (auto v : node) {
			Value value;
			NodePath newNodePath = nodePath;
			auto key = v.first.as<std::string>();
			newNodePath.emplace_back(key);
			auto keyTyped = from_string<Key>(key);
			serializer.deserialize(v.second, _map[keyTyped], newNodePath);
		}
	}
}




}
}

