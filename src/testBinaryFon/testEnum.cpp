#include <catch/catch.hpp>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>

namespace {

template <typename ...Ts>
auto createVector(Ts... ts) {
	return std::vector<std::byte>{std::byte{uint8_t(ts)}...};
}


enum class MyEnum : int32_t {
	E1,
	E2,
	E3
};

TEST_CASE("test binary serialization of enum", "[binary][enum]") {
	SECTION("E1") {
		auto data = MyEnum::E1;
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, std::byte(MyEnum::E1), 0, 0, 0
		);
		REQUIRE(buffer == expect);
	}
	SECTION("E2") {
		auto data = MyEnum::E2;
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, std::byte(MyEnum::E2), 0, 0, 0
		);
		REQUIRE(buffer == expect);
	}
	SECTION("E3") {
		auto data = MyEnum::E3;
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, std::byte(MyEnum::E3), 0, 0, 0
		);
		REQUIRE(buffer == expect);
	}
}
TEST_CASE("test binary deserialization of enum", "[binary][enum]") {
	SECTION("E1") {
		auto buffer = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, std::byte(MyEnum::E1), 0, 0, 0
		);
		auto data = fon::binary::deserialize<MyEnum>(buffer);
		REQUIRE(data == MyEnum::E1);
	}
	SECTION("E2") {
		auto buffer = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, std::byte(MyEnum::E2), 0, 0, 0
		);
		auto data = fon::binary::deserialize<MyEnum>(buffer);
		REQUIRE(data == MyEnum::E2);
	}
	SECTION("E3") {
		auto buffer = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, std::byte(MyEnum::E3), 0, 0, 0
		);
		auto data = fon::binary::deserialize<MyEnum>(buffer);
		REQUIRE(data == MyEnum::E3);
	}
}
}
