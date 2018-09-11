#include "Serializer.h"

namespace serializer {
namespace yaml {

SerializerNodeInput::SerializerNodeInput(Serializer& _serializer, std::string const& _name, YAML::Node& _node, NodePath const& _nodePath)
	: serializer ( _serializer )
	, name       { _name }
	, node       ( _node )
	, nodePath   { _nodePath }
{
}



SerializerNode::SerializerNode(Serializer& _serializer, YAML::Node& _node, NodePath const& _nodePath)
	: serializer ( _serializer )
	, node       ( _node )
	, nodePath   { _nodePath }
{
	auto const& m = _serializer.getUnusedFields();
	auto iter = m.find(to_string(_nodePath));
	if (iter != m.end()) {
		node = iter->second;
	}
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
		YAML::Node values;
		serialize(values, nodePaths, {});
		node["__ownerlessObjects"] = values;
	}
	if (not sharedToId.empty()) {
		node["__sharedObjects"] = sharedObjectNode;
	}
}




}
}
