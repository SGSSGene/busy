#include "Deserializer.h"

#include "../standardTypes.h"



namespace serializer {
namespace json {

DeserializerNode::DeserializerNode(Deserializer& _serializer, Json::Value& _node, bool _available, NodePath const& _nodePath)
	: serializer ( _serializer )
	, node       ( _node )
	, nodePath   { _nodePath }
{
	(void)_available; // Unused
}

DeserializerNode::~DeserializerNode() {
}

DeserializerNodeInput DeserializerNode::operator[](std::string const& _str) {
	NodePath newNodePath = nodePath;
	newNodePath.push_back(_str);
	if (not node.isMember(_str)) {
		return DeserializerNodeInput(serializer, node, false, newNodePath);
	}

	return DeserializerNodeInput(serializer, node[_str], true, newNodePath);
}

Deserializer::Deserializer(std::vector<uint8_t> const& _data) {
	Json::Reader reader;
	std::string str((char*)_data.data());
	if (not reader.parse(str, node)) {
		throw std::runtime_error("Parsing file failed: "+reader.getFormattedErrorMessages());
	}
	if (node.isObject() and node.isMember("__sharedObjects")) {
		sharedObjectNode = node["__sharedObjects"];
	}

}
Deserializer::Deserializer(std::string const& _data) {
	Json::Reader reader;
	std::string str(_data);
	if (not reader.parse(str, node)) {
		throw std::runtime_error("Parsing file failed: "+reader.getFormattedErrorMessages());
	}
	if (node.isObject() and node.isMember("__sharedObjects")) {
		sharedObjectNode = node["__sharedObjects"];
	}
}

void Deserializer::close() {
	// Check if ownerless objects (raw pointers table) are available
	std::map<int32_t, std::vector<NodePath>> nodePaths;

	if (node.isObject() and node.isMember("__ownerlessObjects")) {
		Json::Value values = node["__ownerlessObjects"];
		deserialize(values, nodePaths, {"__ownerlessObjects"});
	}

	for (auto& raw : rawAddresses) {
		if (nodePaths.find(raw.first) == nodePaths.end()) continue;
		auto const& paths = nodePaths[raw.first];

		for (auto const& path : paths) {
			if (knownAddresses.find(path) != knownAddresses.end()) {
				void const* value = knownAddresses[path].ptr;
				for (void* p : raw.second.needOverwrite) {
					memcpy(p, &value, sizeof(void*));
				}
//				break;
			}
		}
	}
}




}
}
