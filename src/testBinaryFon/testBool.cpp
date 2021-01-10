#include <catch/catch.hpp>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>

namespace {
template <typename ...Ts>
auto createVector(Ts... ts) {
	return std::vector<std::byte>{std::byte{uint8_t(ts)}...};
}

TEST_CASE("test binary serialization of bool", "[binary][bool]") {
	SECTION ("testing 'true'") {
		bool data {true};
		auto buffer = fon::binary::serialize(data);

		REQUIRE(buffer.size() == 10);
		auto expect = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, 1
		);
		CHECK(buffer == expect);
	}

	SECTION ("testing 'false'") {
		bool data {false};
		auto buffer = fon::binary::serialize(data);

		REQUIRE(buffer.size() == 10);
		auto expect = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, 0
		);
		CHECK(buffer == expect);
	}
}

TEST_CASE("test binary deserialization of bool", "[binary][bool]") {
	SECTION ("testing 'true'") {
		auto buffer = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, 1
		);
		auto data = fon::binary::deserialize<bool>(buffer);
		REQUIRE(data == true);
	}

	SECTION ("testing 'false'") {
		auto buffer = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, 0
		);
		auto data = fon::binary::deserialize<bool>(buffer);
		REQUIRE(data == false);
	}
}
}
