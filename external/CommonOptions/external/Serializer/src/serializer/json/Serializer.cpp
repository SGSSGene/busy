#include "Serializer.h"

namespace serializer {
namespace json {

SerializerNodeInput::SerializerNodeInput(Serializer& _serializer, std::string const& _name, Json::Value& _node, NodePath const& _nodePath)
	: serializer ( _serializer )
	, name       { _name }
	, node       ( _node )
	, nodePath   { _nodePath }
{
}



SerializerNode::SerializerNode(Serializer& _serializer, Json::Value& _node, NodePath const& _nodePath)
	: serializer ( _serializer )
	, node       ( _node )
	, nodePath   { _nodePath }
{
}

SerializerNode::~SerializerNode() {
}

SerializerNodeInput SerializerNode::operator[](std::string const& _str) {
	NodePath path = nodePath;
	path.push_back(_str);

	SerializerNodeInput input(serializer, _str, node, path);

	return input;
}

void Serializer::close() {
	if (rawAddresses.size() > 0) {
		std::map<int32_t, std::vector<NodePath>> nodePaths;
		for (auto const& raw : rawAddresses) {
			// find raw
			for (auto const& known : knownAddresses) {
				if (known.first == raw.first) {
					auto& path = known.second.bufferPos;
					nodePaths[raw.second.ptrId] = path;
					break;
				}
			}
		}
		Json::Value values;
		serialize(values, nodePaths, {});
		node["__ownerlessObjects"] = values;
	}
	if (not sharedToId.empty()) {
		node["__sharedObjects"] = sharedObjectNode;
	}
}




}
}
