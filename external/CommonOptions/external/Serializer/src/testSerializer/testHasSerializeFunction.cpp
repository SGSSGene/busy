#include <serializer/serializer.h>
#include <gtest/gtest.h>

struct HasSerializeFunctionA {
	std::vector<int> x {5, 6};
	HasSerializeFunctionA() {
	}
	template <typename Node>
	void serialize(Node& node) {
		node["x"] % x;
	}
};

struct HasSerializeFunctionB {};

struct HasSerializeFunctionC {
	template <typename Node, typename std::enable_if<serializer::is_serializable<Node, HasSerializeFunctionA>::value>::type* = nullptr>
	void serialize(Node& node) {
	}
};
struct HasSerializeFunctionD {
	template <typename Node, typename std::enable_if<serializer::is_serializable<Node, HasSerializeFunctionB>::value>::type* = nullptr>
	void serialize(Node& node) {
	}
};

template <typename T>
struct HasSerializeFunctionE {
	std::vector<T> data;

	template <typename Node, typename std::enable_if<serializer::is_serializable<Node, std::vector<T>>::value>::type* = nullptr>
	void serialize(Node& node) {
	}
};

TEST(TestHasSerializeFunction, TestHasSerialize) {
	HasSerializeFunctionA a;
	auto data = serializer::yaml::writeAsString(a);

	EXPECT_TRUE(serializer::is_serializable<bool>::value);
	EXPECT_TRUE(serializer::is_serializable<char>::value);
	EXPECT_TRUE(serializer::is_serializable<uint8_t>::value);
	EXPECT_TRUE(serializer::is_serializable<int8_t>::value);
	EXPECT_TRUE(serializer::is_serializable<uint16_t>::value);
	EXPECT_TRUE(serializer::is_serializable<int16_t>::value);
	EXPECT_TRUE(serializer::is_serializable<uint32_t>::value);
	EXPECT_TRUE(serializer::is_serializable<int32_t>::value);
	EXPECT_TRUE(serializer::is_serializable<uint64_t>::value);
	EXPECT_TRUE(serializer::is_serializable<int64_t>::value);
	EXPECT_TRUE(serializer::is_serializable<float>::value);
	EXPECT_TRUE(serializer::is_serializable<double>::value);

	using Pair = std::pair<bool, int>;
	EXPECT_TRUE(serializer::is_serializable<Pair>::value);

	EXPECT_TRUE(serializer::is_serializable<std::vector<bool>>::value);
	EXPECT_TRUE(serializer::is_serializable<std::list<bool>>::value);
	EXPECT_TRUE(serializer::is_serializable<std::set<bool>>::value);
	EXPECT_TRUE(serializer::is_serializable<std::vector<std::string>>::value);
	using Map = std::map<std::string, int>;
	EXPECT_TRUE(serializer::is_serializable<Map>::value);

	EXPECT_TRUE(serializer::is_serializable<HasSerializeFunctionA>::value);
	EXPECT_FALSE(serializer::is_serializable<HasSerializeFunctionB>::value);
	EXPECT_TRUE(serializer::is_serializable<HasSerializeFunctionC>::value);
	EXPECT_FALSE(serializer::is_serializable<HasSerializeFunctionD>::value);

	using T1 = std::vector<HasSerializeFunctionD>;
	EXPECT_FALSE(serializer::is_serializable<T1>::value);
	using T2 = std::vector<HasSerializeFunctionC>;
	EXPECT_TRUE(serializer::is_serializable<T2>::value);

	EXPECT_TRUE(serializer::is_serializable<std::vector<int>>::value);

	using T5 = serializer::is_serializable<serializer::binary::SerializerNode, HasSerializeFunctionE<int>>;
	EXPECT_TRUE(T5::value);
	EXPECT_FALSE(serializer::is_serializable<HasSerializeFunctionE<HasSerializeFunctionD>>::value);

	using T3 = std::vector<HasSerializeFunctionE<HasSerializeFunctionD>>;
	EXPECT_FALSE(serializer::is_serializable<T3>::value);
	using T4 = std::vector<HasSerializeFunctionE<HasSerializeFunctionC>>;
	EXPECT_TRUE(serializer::is_serializable<T4>::value);



}


