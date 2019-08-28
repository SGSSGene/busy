#pragma once

#include <json/json.h>
#include <fstream>
#include <sstream>

#include "Deserializer.h"
#include "../standardTypes.h"

#ifdef BUSY_GENERICFACTORY
	#include <genericFactory/genericFactory.h>
#endif

#include "../is_copy_constructible_recursive.h"

namespace serializer {
namespace json {

template<typename T>
std::string to_string(T const& _value) {
	return std::to_string(_value);
}
inline std::string to_string(std::string const& _value) {
	return _value;
}


class Serializer;

template<typename T>
class SerializerDefault {
private:
	Serializer&  serializer;
	std::string  name;
	Json::Value& node;
	Json::Value  defaultNode;
	bool         defaultValueGiven;
	T&           value;
	NodePath     nodePath;

public:
	SerializerDefault(Serializer& _serializer, std::string const& _name, Json::Value& _node, T& _value, NodePath const& _nodePath)
		: serializer ( _serializer  )
		, name       { _name }
		, node       ( _node )
		, defaultValueGiven { false }
		, value      ( _value )
		, nodePath   { _nodePath }
	{ }
	~SerializerDefault();

	void operator or(T const& t) {
		setDefault(t);
	}

	template<typename T2, typename std::enable_if<is_copy_constructible_recursive<T2>::value>::type* = nullptr>
	void setDefault(T2 const& t);

	template<typename T2, typename std::enable_if<not is_copy_constructible_recursive<T2>::value>::type* = nullptr>
	void setDefault(T2 const& t);

	template<typename T2, typename std::enable_if<std::is_default_constructible<T2>::value
	                                              and std::is_assignable<T2&, T2>::value>::type* = nullptr>
	void setDefault() {
		*this or T2();
	}
	template<typename T2, typename std::enable_if<not std::is_default_constructible<T2>::value
	                                              or not std::is_assignable<T2&, T2>::value>::type* = nullptr>
	void setDefault() {
	}

};

class SerializerNodeInput {
private:
	Serializer&  serializer;
	std::string  name;
	Json::Value& node;
	NodePath     nodePath;
public:
	SerializerNodeInput(Serializer& _serializer, std::string const& _name, Json::Value& _node, NodePath const& _path);

	template<typename T>
	SerializerDefault<T> operator%(T& t);
};

class SerializerNode {
private:
	Serializer&      serializer;
	std::vector<int> index;
	Json::Value&     node;
	NodePath         nodePath;

public:
	SerializerNode(Serializer& _serialize, Json::Value& _node, NodePath const& _nodePath);
	~SerializerNode();
	SerializerNodeInput operator[](std::string const& _str);
};

struct SerializerAdapter {
	Serializer&  serializer;
	Json::Value& node;
	NodePath     nodePath;
	SerializerAdapter(Serializer& _serializer, Json::Value& _node, NodePath const& _nodePath)
		: serializer ( _serializer )
		, node       ( _node )
		, nodePath   { _nodePath }
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
	Json::Value node;

	SerializerNodeInput rootNode {*this, "", node, {}};

	struct KnownAddress {
		std::vector<NodePath>    bufferPos; // Position where this address was serialized
	};
	std::map<void const*, KnownAddress> knownAddresses;
	struct RawAddress {
		int32_t     ptrId;
	};
	std::map<void const*, RawAddress> rawAddresses;

	Json::Value sharedObjectNode { Json::arrayValue };
	std::map<void const*, int32_t> sharedToId;

public:
	Serializer() {
		node = Json::objectValue;
	}
	void close();

	std::vector<uint8_t> getData() const {
		Json::StyledWriter jsonWriter;
		std::string jsonString = jsonWriter.write(node);
		std::vector<uint8_t> retList(jsonString.size()+1);
		memcpy(retList.data(), jsonString.c_str(), retList.size());
		return retList;
	}

	std::string getDataAsStr() const {
		Json::StyledWriter jsonWriter;
		return jsonWriter.write(node);
	}

	SerializerNodeInput const& getRootNode() const {
		return rootNode;
	}
	SerializerNodeInput& getRootNode() {
		return rootNode;
	}
	Json::Value const& getNode() const {
		return node;
	}

	void addKnownAddress(void const* _ptr, NodePath const& _bufferPos) {
		knownAddresses[_ptr].bufferPos.push_back(_bufferPos);
	}

	template<typename T>
	int32_t addSharedObject(std::shared_ptr<T>& _value) {
		if (sharedToId.find(_value.get()) == sharedToId.end()) {
			int32_t id = sharedObjectNode.size();
			sharedToId[_value.get()] = id;
			sharedObjectNode.append(Json::Value{});
			Json::Value value;

			// Extrem hacky!!! casting shared object to unique to use it's serialization
			std::unique_ptr<T> ptr (_value.get());
			serialize(value, ptr, {"__sharedObjects"});
			ptr.release();

			sharedObjectNode[id] = value;

		}
		return sharedToId.at(_value.get());
	}

	template<typename T, typename std::enable_if<std::is_same<T, bool>::value
	                                             or std::is_same<T, uint8_t>::value
	                                             or std::is_same<T, int8_t>::value
	                                             or std::is_same<T, uint16_t>::value
	                                             or std::is_same<T, int16_t>::value
	                                             or std::is_same<T, uint16_t>::value
	                                             or std::is_same<T, int32_t>::value
	                                             or std::is_same<T, uint32_t>::value
	                                             or std::is_same<T, float>::value
	                                             or std::is_same<T, double>::value>::type* = nullptr>
	void serialize(Json::Value& _node, T const& _value, NodePath const& _nodePath) {
		_node = Json::Value(_value);
		addKnownAddress(&_value, _nodePath);
	}
	template<typename T, typename std::enable_if<std::is_same<T, int64_t>::value>::type* = nullptr>
	void serialize(Json::Value& _node, T const& _value, NodePath const& _nodePath) {
		_node = Json::Value::Int64(_value);
		addKnownAddress(&_value, _nodePath);
	}
	template<typename T, typename std::enable_if<std::is_same<T, uint64_t>::value>::type* = nullptr>
	void serialize(Json::Value& _node, T const& _value, NodePath const& _nodePath) {
		_node = Json::Value::UInt64(_value);
		addKnownAddress(&_value, _nodePath);
	}
	template<typename T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
	void serialize(Json::Value& _node, T const& _value, NodePath const& _nodePath) {
		_node = _value;

		addKnownAddress(&_value, _nodePath);
	}

	template<typename T, typename std::enable_if<has_serialize_function<T, SerializerNode>::value>::type* = nullptr>
	void serialize(Json::Value& _node, T& _value, NodePath const& _nodePath) {

		addKnownAddress(&_value, _nodePath);

		SerializerNode node(*this, _node, _nodePath);

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
	void serialize(Json::Value& _node, T*& _value, NodePath const& _nodePath) {
		auto iter = rawAddresses.find(_value);
		if (iter == rawAddresses.end()) {
			int32_t ptrId = rawAddresses.size();
			rawAddresses[_value] = {ptrId};
			iter = rawAddresses.find(_value);
		}
		int32_t ptrId = iter->second.ptrId;
		serialize(_node, ptrId, _nodePath);
		addKnownAddress(&_value, _nodePath);
	}

	template<typename T>
	void serialize(Json::Value& _node, std::shared_ptr<T>& _value, NodePath const& _nodePath) {
		int32_t ptrId = {-1};
		if (_value.get() != nullptr) {
			ptrId = addSharedObject(_value);
		}
		serialize(_node, ptrId, _nodePath);
	}

	template<typename T, typename std::enable_if<not std::is_fundamental<T>::value
	                                             and not std::is_same<T, std::string>::value
	                                             and not has_serialize_function<T, SerializerNode>::value>::type* = nullptr>
	void serialize(Json::Value& _node, T& _value, NodePath const& _nodePath) {

		addKnownAddress(&_value, _nodePath);
		SerializerAdapter adapter(*this, _node, _nodePath);
		Converter<T>::serialize(adapter, _value);
	}
};

template<typename T>
void read(std::string const& _file, T& _value);

template<typename T>
void write(std::string const& _file, T& _value);

template<typename T>
SerializerDefault<T>::~SerializerDefault() {
	if (name == "") {
		serializer.serialize(node, value, nodePath);
		return;
	}

	serializer.serialize(node[name], value, nodePath);
	if (not defaultValueGiven) {
		setDefault<T>();
	}

	if (defaultValueGiven and node[name] == defaultNode) {
		node.removeMember(name);
	}
}
template<typename T>
template<typename T2, typename std::enable_if<is_copy_constructible_recursive<T2>::value>::type*>
void SerializerDefault<T>::setDefault(T2 const& t) {
	try {
		Serializer ser;
		T p (t);
		ser.getRootNode() % p;
		ser.close();
		defaultNode = ser.getNode();
		defaultValueGiven = true;
	} catch (...) {
	}
}

template<typename T>
template<typename T2, typename std::enable_if<not is_copy_constructible_recursive<T2>::value>::type*>
void SerializerDefault<T>::setDefault(T2 const&) {
}



template<typename T>
SerializerDefault<T> SerializerNodeInput::operator%(T& t) {
	return SerializerDefault<T>(serializer, name, node, t, nodePath);
}

template<typename T>
void SerializerAdapter::serialize(T& _value) {
	serializer.serialize(node, _value, nodePath);
}

template<typename Iter>
void SerializerAdapter::serializeByIter(Iter iter, Iter end) {
	node = Json::Value(Json::arrayValue);
	int32_t index { 0 };
	for (; iter != end; ++iter) {
		auto newNodePath = nodePath;
		newNodePath.push_back(std::to_string(index++));

		Json::Value tnode;
		serializer.serialize(tnode, *iter, newNodePath);
		node.append(tnode);
	}
}
template<typename Iter>
void SerializerAdapter::serializeMapByIter(Iter iter, Iter end) {
	node = Json::Value(Json::objectValue);
	for (; iter != end; ++iter) {
		auto newNodePath = nodePath;
		newNodePath.push_back(to_string(iter->first));

		Json::Value tnode;
		serializer.serialize(tnode, iter->second, newNodePath);
		node[to_string(iter->first)] = tnode;
	}
}


template<typename Iter>
void SerializerAdapter::serializeByIterCopy(Iter iter, Iter end) {
	node = Json::Value(Json::arrayValue);
	int32_t index { 0 };
	for (; iter != end; ++iter) {
		auto newNodePath = nodePath;
		newNodePath.push_back(std::to_string(index++));

		Json::Value tnode;
		auto t = *iter;
		serializer.serialize(tnode, t, newNodePath);
		node.append(tnode);
	}
}


template<typename T>
void read(std::string const& _file, T& _value) {

	// Read file from storage
	std::ifstream ifs(_file);
	if (ifs.fail()) {
		throw std::runtime_error("Opening file failed");
	}
	std::stringstream strStream;
	strStream << ifs.rdbuf();

	// parse file in serializer
	Deserializer serializer(strStream.str());
	serializer.getRootNode() % _value;
	serializer.close();
}

template<typename T>
void write(std::string const& _file, T& _value) {
	// Serialize data
	Serializer serializer;
	serializer.getRootNode() % _value;
	serializer.close();

	std::ofstream oFile(_file);
	if (oFile.fail()) {
		throw std::runtime_error("Opening file failed");
	}
	oFile << serializer.getDataAsStr();
	oFile.close();
	if (oFile.fail()) {
		throw std::runtime_error("Writing to file failed");
	}
}


}
}


