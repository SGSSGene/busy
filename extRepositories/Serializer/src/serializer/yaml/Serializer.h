#pragma once

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <sstream>

#include "Deserializer.h"
#include <serializer/standardTypes.h>

#ifdef BUSY_GENERICFACTORY
	#include <genericFactory/genericFactory.h>
#endif

#include <serializer/is_copy_constructible_recursive.h>

namespace serializer {
namespace yaml {

struct StringConvertTo {
	static std::string to(std::string const& _value) {
		return _value;
	}
	template <typename T, typename std::enable_if<not std::is_enum<T>::value>::type* = nullptr>
	static std::string to(T const& _value) {
		return std::to_string(_value);
	}

	template <typename T, typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
	static std::string to(T const& _value) {
		using Type = typename std::underlying_type<T>::type;
		return to(Type(_value));
	}
};
template<typename T>
std::string to_string(T const& _value) {
	return StringConvertTo::to(_value);
}

class Serializer;

template<typename T>
class SerializerDefault {
private:
	Serializer&  serializer;
	std::string  name;
	YAML::Node&  node;
	YAML::Node   defaultNode;
	bool         defaultValueGiven;
	T&           value;
	NodePath     nodePath;

public:
	SerializerDefault(Serializer& _serializer, std::string const& _name, YAML::Node& _node, T& _value, NodePath const& _nodePath)
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
	YAML::Node&  node;
	NodePath     nodePath;
public:
	SerializerNodeInput(Serializer& _serializer, std::string const& _name, YAML::Node& _node, NodePath const& _path);

	template<typename T>
	SerializerDefault<T> operator%(T& t);
};

class SerializerNode {
private:
	Serializer&      serializer;
	std::vector<int> index;
	YAML::Node&      node;
	NodePath         nodePath;

public:
	SerializerNode(Serializer& _serialize, YAML::Node& _node, NodePath const& _nodePath);
	~SerializerNode();
	SerializerNodeInput operator[](std::string const& _str);
};

struct SerializerAdapter {
	Serializer&  serializer;
	YAML::Node&  node;
	NodePath     nodePath;
	SerializerAdapter(Serializer& _serializer, YAML::Node& _node, NodePath const& _nodePath)
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



	template<typename T>
	static constexpr bool isSimpleType() {
		using NR = typename std::remove_reference<T>::type;
		return std::is_same<NR, bool>::value
		       or std::is_same<NR, uint8_t>::value
		       or std::is_same<NR, int8_t>::value
		       or std::is_same<NR, uint16_t>::value
		       or std::is_same<NR, int16_t>::value
		       or std::is_same<NR, uint32_t>::value
		       or std::is_same<NR, int32_t>::value
		       or std::is_same<NR, float>::value
		       or std::is_same<NR, double>::value;
	}
};


class Serializer {
	YAML::Node  node;

	SerializerNodeInput rootNode {*this, "", node, {}};

	struct KnownAddress {
		std::vector<NodePath>    bufferPos; // Position where this address was serialized
	};
	std::map<void const*, KnownAddress> knownAddresses;
	struct RawAddress {
		int32_t     ptrId;
	};
	std::map<void const*, RawAddress> rawAddresses;

	YAML::Node  sharedObjectNode; //!TODO needs initialization?

	std::map<void const*, int32_t> sharedToId;

	bool mNoDefault {false};

	std::map<std::string, YAML::Node> mUnusedFields;
public:
	Serializer() {
		//node //!TODO needs initialization?
	}
	void setNoDefault(bool _noDefault) {
		mNoDefault = _noDefault;
	}
	bool getNoDefault() const {
		return mNoDefault;
	}
	void setUnusedFields(std::map<std::string, YAML::Node> _unusedFields) {
		mUnusedFields = _unusedFields;
	}
	auto getUnusedFields() const -> std::map<std::string, YAML::Node> const& {
		return mUnusedFields;
	}

	void close();

	std::vector<uint8_t> getData() const {
		YAML::Emitter emitter;
		emitter.SetFloatPrecision(std::numeric_limits<float>::digits10+2);
		emitter.SetDoublePrecision(std::numeric_limits<double>::digits10+2);
		emitter << node;
		emitter.c_str();
		std::vector<uint8_t> retList(emitter.size()+1);
		memcpy(retList.data(), emitter.c_str(), retList.size());
		return retList;
	}

	std::string getDataAsStr() const {
		YAML::Emitter emitter;
		emitter << node;
		return emitter.c_str();
	}

	SerializerNodeInput const& getRootNode() const {
		return rootNode;
	}
	SerializerNodeInput& getRootNode() {
		return rootNode;
	}
	YAML::Node const& getNode() const {
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

			{ //Yaml conversion
				sharedObjectNode.push_back(YAML::Node{});
				YAML::Node value;

				// Extrem hacky!!! casting shared object to unique to use it's serialization
				std::unique_ptr<T> ptr (_value.get());
				serialize(value, ptr, {"__sharedObjects"});
				ptr.release();

				sharedObjectNode[id] = value;

			}

		}
		return sharedToId.at(_value.get());
	}

	template<typename T, typename std::enable_if<std::is_same<T, uint8_t>::value
	                                             or std::is_same<T, int8_t>::value>::type* = nullptr>
	void serialize(YAML::Node& _node, T const& _value, NodePath const& _nodePath) {
		_node = YAML::Node((int16_t)_value);
		addKnownAddress(&_value, _nodePath);
	}

	template<typename T, typename std::enable_if<std::is_same<T, bool>::value
	                                             or std::is_same<T, uint16_t>::value
	                                             or std::is_same<T, int16_t>::value
	                                             or std::is_same<T, uint16_t>::value
	                                             or std::is_same<T, int32_t>::value
	                                             or std::is_same<T, uint32_t>::value
	                                             or std::is_same<T, int64_t>::value
	                                             or std::is_same<T, uint64_t>::value
	                                             or std::is_same<T, float>::value
	                                             or std::is_same<T, double>::value>::type* = nullptr>
	void serialize(YAML::Node& _node, T const& _value, NodePath const& _nodePath) {
		_node = YAML::Node(_value);
		addKnownAddress(&_value, _nodePath);
	}
	template<typename T, typename std::enable_if<std::is_same<T, std::string>::value>::type* = nullptr>
	void serialize(YAML::Node& _node, T const& _value, NodePath const& _nodePath) {
		_node = _value;

		addKnownAddress(&_value, _nodePath);
	}

	template<typename T, typename std::enable_if<has_serialize_function<T, SerializerNode>::value>::type* = nullptr>
	void serialize(YAML::Node& _node, T& _value, NodePath const& _nodePath) {

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
	void serialize(YAML::Node& _node, T*& _value, NodePath const& _nodePath) {
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
	void serialize(YAML::Node& _node, std::shared_ptr<T>& _value, NodePath const& _nodePath) {
		int32_t ptrId = {-1};
		if (_value.get() != nullptr) {
			ptrId = addSharedObject(_value);
		}
		serialize(_node, ptrId, _nodePath);
	}

	template<typename T, typename std::enable_if<not std::is_fundamental<T>::value
	                                             and not std::is_same<T, std::string>::value
	                                             and not has_serialize_function<T, SerializerNode>::value>::type* = nullptr>
	void serialize(YAML::Node& _node, T& _value, NodePath const& _nodePath) {

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

	auto tnode = node[name];
	serializer.serialize(tnode, value, nodePath);

	if (not defaultValueGiven) {
		setDefault<T>();
	}

	if (defaultValueGiven) {
		YAML::Emitter em1, em2;
		em1 << node[name];
		em2 << defaultNode;

		if (strcmp(em1.c_str(), em2.c_str()) == 0
		   and not serializer.getNoDefault()) {
			node.remove(name);
		}
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
	int32_t index { 0 };
	for (; iter != end; ++iter) {
		auto newNodePath = nodePath;
		newNodePath.push_back(std::to_string(index++));

		YAML::Node tnode;
		serializer.serialize(tnode, *iter, newNodePath);
		node.push_back(tnode);
	}
	if (index == 0 or isSimpleType<decltype(*iter)>()) { // No enty in the list, print at least empty list
		node.SetStyle(YAML::EmitterStyle::Flow);
	}
}
template<typename Iter>
void SerializerAdapter::serializeMapByIter(Iter iter, Iter end) {
	for (; iter != end; ++iter) {
		auto newNodePath = nodePath;
		auto key = to_string(iter->first);
		newNodePath.push_back(key);

		YAML::Node tnode;
		serializer.serialize(tnode, iter->second, newNodePath);
		node[key] = tnode;
	}
}


template<typename Iter>
void SerializerAdapter::serializeByIterCopy(Iter iter, Iter end) {
	int32_t index { 0 };
	for (; iter != end; ++iter) {
		auto newNodePath = nodePath;
		newNodePath.push_back(std::to_string(index++));

		YAML::Node tnode;
		auto t = *iter;
		serializer.serialize(tnode, *iter, newNodePath);
		node.push_back(tnode);
	}

	if (index == 0 or isSimpleType<decltype(*iter)>()) { // No enty in the list, print at least empty list
		node.SetStyle(YAML::EmitterStyle::Flow);
	}
}


template<typename T>
void read(std::string const& _file, T& _value) {

	// Read file from storage
	std::ifstream ifs(_file);
	if (ifs.fail()) {
		throw std::runtime_error("Opening file failed: " + _file);
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
		throw std::runtime_error("Opening file failed: " + _file);
	}
	oFile << serializer.getDataAsStr() << "\n";
	oFile.close();
	if (oFile.fail()) {
		throw std::runtime_error("Writing to file failed: " + _file);
	}
}

template<typename T>
std::string writeAsString(T& _value, std::map<std::string, YAML::Node> const* unusedFields = nullptr) {
	// Serialize data
	Serializer serializer;
	if (unusedFields != nullptr) {
		serializer.setUnusedFields(*unusedFields);
	}
	serializer.getRootNode() % _value;
	serializer.close();

	return serializer.getDataAsStr();
}

template<typename T>
void readFromString(std::string const& _str, T& _value, std::map<std::string, YAML::Node>* unusedFields = nullptr) {

	// parse file in serializer
	Deserializer serializer(_str);
	serializer.getRootNode() % _value;
	serializer.close();
	if (unusedFields != nullptr) {
		*unusedFields = serializer.getUnusedFields();
	}
}





}
}


