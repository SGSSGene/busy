#include "Serializer.h"



namespace serializer {
namespace binary {

SerializerNodeInput::SerializerNodeInput(SerializerNodeInput const& _other)
	: serializer ( _other.serializer )
	, id         { _other.id }
	, needToKnowAddress { _other.needToKnowAddress }
{
	serializer.mCurrentPath.push_back("");
}


SerializerNodeInput::SerializerNodeInput(Serializer& _serializer, int _id, bool _needToKnowAddress, std::string const& _str)
	: serializer ( _serializer )
	, id         { _id }
	, needToKnowAddress { _needToKnowAddress }
{
	if (_str != "") {
		serializer.mCurrentPath.push_back(_str);
	}
}



SerializerNodeInput::~SerializerNodeInput() {
	if (not serializer.mCurrentPath.empty()) {
		serializer.mCurrentPath.pop_back();
	}
}




SerializerNode::SerializerNode(Serializer& _serializer, bool _needToKnowAddress)
	: serializer ( _serializer )
	, needToKnowAddress { _needToKnowAddress }
{
	serializer.serialize(int32_t(), false);
	startPoint = serializer.getCurrentPosition();
}

SerializerNode::~SerializerNode() {
	// check which values are missing
	auto& map = serializer.mUnusedFields[to_string(serializer.mCurrentPath)];
	for (auto const& e : map) {
		if (mAccessed.count(e.first) == 0) {
			int32_t id = serializer.mapStringToInt(e.first);
			serializer.serialize(id, false);
			serializer.serialize(e.second, false);
		}
	}

	
	// write total size
	int endPoint = serializer.getCurrentPosition();
	int32_t size = endPoint - startPoint;
	memcpy(serializer.getPtr() + startPoint - sizeof(int32_t), &size, sizeof(size));
}

SerializerNodeInput SerializerNode::operator[](std::string const& _str) {
	int32_t id = serializer.mapStringToInt(_str);

	mAccessed.insert(_str);
	SerializerNodeInput input(serializer, id, needToKnowAddress, _str);

	return input;
}



}
}
