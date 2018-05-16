#include "Deserializer.h"

namespace serializer {
namespace binary {

DeserializerNodeInput::DeserializerNodeInput(DeserializerNodeInput const& _other)
	: serializer ( _other.serializer )
	, available  { _other.available}
	, needToKnowAddress { _other.needToKnowAddress }
{
	serializer.mCurrentPath.push_back("");
}


DeserializerNodeInput::DeserializerNodeInput(Deserializer& _serializer, bool _available, bool _needToKnowAddress, std::string const& _str)
	: serializer ( _serializer )
	, available  { _available}
	, needToKnowAddress { _needToKnowAddress }
{
	if (_str != "") {
		serializer.mCurrentPath.push_back(_str);
	}
}


DeserializerNodeInput::~DeserializerNodeInput() {
	if (not serializer.mCurrentPath.empty()) {
		serializer.mCurrentPath.pop_back();
	}
}


DeserializerNode::DeserializerNode(Deserializer& _serializer, bool _available, bool _needToKnowAddress)
	: serializer ( _serializer )
	, needToKnowAddress { _needToKnowAddress }
{
	(void)_available;
	serializer.deserialize(size, false);
	startPoint = serializer.getCurrentPosition();
}

DeserializerNode::~DeserializerNode() {
	// check all entries

	serializer.setCurrentPosition(startPoint);
	while (serializer.getCurrentPosition() < startPoint + size) {
		int32_t id;
		serializer.deserialize(id, false);

		auto str = serializer.mapIntToString(id);

		if (mAccessed.count(to_string(serializer.mCurrentPath, str)) == 0) {
			auto& map = serializer.mUnusedFields[to_string(serializer.mCurrentPath)];
			serializer.deserialize(map[str], false);
		} else {

			int32_t member_size;
			serializer.deserialize(member_size, false);

			serializer.setCurrentPosition(serializer.getCurrentPosition() + member_size);
		}
	}

	
	// check pointer after this node
	serializer.setCurrentPosition(startPoint + size);
}

DeserializerNodeInput DeserializerNode::operator[](std::string const& _str) {
	serializer.setCurrentPosition(startPoint);

	mAccessed.insert(_str);

	while (serializer.getCurrentPosition() < startPoint + size) {
		int32_t id;
		serializer.deserialize(id, false);
		int32_t member_size;
		serializer.deserialize(member_size, false);

		auto str = serializer.mapIntToString(id);
		if (str == _str) {
			DeserializerNodeInput input(serializer, true, needToKnowAddress, _str);
			return input;
		}
		serializer.setCurrentPosition(serializer.getCurrentPosition() + member_size);
	}

	DeserializerNodeInput input(serializer, false, needToKnowAddress, _str);
	return input;
}


}
}
