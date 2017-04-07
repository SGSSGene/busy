#include "Serializer.h"



namespace serializer {
namespace binary {

SerializerNodeInput::SerializerNodeInput(Serializer& _serializer, int _id, bool _needToKnowAddress)
	: serializer ( _serializer )
	, id         { _id }
	, needToKnowAddress { _needToKnowAddress }
{
}



SerializerNode::SerializerNode(Serializer& _serializer, bool _needToKnowAddress)
	: serializer ( _serializer )
	, needToKnowAddress { _needToKnowAddress }
{
	serializer.serialize(int32_t(), false);
	startPoint = serializer.getCurrentPosition();
}

SerializerNode::~SerializerNode() {
	int endPoint = serializer.getCurrentPosition();
	int32_t size = endPoint - startPoint;
	memcpy(serializer.getPtr() + startPoint - sizeof(int32_t), &size, sizeof(size));
}

SerializerNodeInput SerializerNode::operator[](std::string const& _str) {
	int32_t id = serializer.mapStringToInt(_str);

	SerializerNodeInput input(serializer, id, needToKnowAddress);

	return input;
}



}
}
