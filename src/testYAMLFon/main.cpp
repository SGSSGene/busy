#include <catch/catch.hpp>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>

struct A {
	template <typename Node>
	void serialize(Node& node) {
	}
};

TEST_CASE("test serializer", "[vector]") {
	std::vector<int32_t> data{10, 20, 30};
	auto node = fon::yaml::serialize(data);
	REQUIRE(node.IsSequence());
	REQUIRE(node.size() == 3);
	REQUIRE(node[0].as<int32_t>() == 10);
	REQUIRE(node[1].as<int32_t>() == 20);
	REQUIRE(node[2].as<int32_t>() == 30);
}
