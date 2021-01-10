#include <catch/catch.hpp>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>

namespace {

template <typename ...Ts>
auto createVector(Ts... ts) {
	return std::vector<std::byte>{std::byte{uint8_t(ts)}...};
}

TEST_CASE("test binary serialization of int8_t", "[binary][int8_t]") {
	SECTION("positive number") {
		int8_t data {7}; // "random" number
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, 7
		);
		REQUIRE(buffer == expect);
	}

	SECTION("small number") {
		int8_t data {-7}; // "random" number
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, -7
		);
		REQUIRE(buffer == expect);
	}

	SECTION("max number") {
		int8_t data {127};
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, 127
		);
		REQUIRE(buffer == expect);
	}

	SECTION("min number") {
		int8_t data {-128};
		auto node = fon::binary::serialize(data);
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, -128
		);
		REQUIRE(buffer == expect);
	}
}
TEST_CASE("test binary deserialization of int8_t", "[binary][int8_t]") {
	SECTION("positive number") {
		auto buffer = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, 7 // "random" number
		);
		auto data = fon::binary::deserialize<int8_t>(buffer);
		REQUIRE(data == 7);
	}
	SECTION("small number") {
		auto buffer = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, -7 // "random" number
		);
		auto data = fon::binary::deserialize<int8_t>(buffer);
		REQUIRE(data == -7);
	}
	SECTION("max number") {
		auto buffer = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, 127
		);
		auto data = fon::binary::deserialize<int8_t>(buffer);
		REQUIRE(data == 127);
	}
	SECTION("min number") {
		auto buffer = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, -128
		);
		auto data = fon::binary::deserialize<int8_t>(buffer);
		REQUIRE(data == -128);
	}
}

TEST_CASE("test binary serialization of uint8_t", "[binary][uint8_t]") {
	SECTION("small number") {
		uint8_t data {7}; // "random" number
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, 7
		);
		REQUIRE(buffer == expect);
	}

	SECTION("max number") {
		uint8_t data {255};
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, 255
		);
		REQUIRE(buffer == expect);
	}

	SECTION("min number") {
		int8_t data {0};
		auto node = fon::binary::serialize(data);
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, 0
		);
		REQUIRE(buffer == expect);
	}
}
TEST_CASE("test binary deserialization of uint8_t", "[binary][uint8_t]") {
	SECTION("small number") {
		auto buffer = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, 7 // "random" number
		);
		auto data = fon::binary::deserialize<uint8_t>(buffer);
		REQUIRE(data == 7);
	}
	SECTION("max number") {
		auto buffer = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, 255
		);
		auto data = fon::binary::deserialize<uint8_t>(buffer);
		REQUIRE(data == 255);
	}
	SECTION("min number") {
		auto buffer = createVector(
		     10, 0, 0, 0, 0, 0, 0, 0,
		      1, 0
		);
		auto data = fon::binary::deserialize<uint8_t>(buffer);
		REQUIRE(data == 0);
	}
}

TEST_CASE("test binary serialization of int16_t", "[binary][int16_t]") {
	SECTION("small number") {
		int16_t data {7}; // "random" number
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     11, 0, 0, 0, 0, 0, 0, 0,
		      1, 7, 0
		);
		REQUIRE(buffer == expect);
	}

	SECTION("max number") {
		auto data {std::numeric_limits<int16_t>::max()};
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     11, 0, 0, 0, 0, 0, 0, 0,
		      1, 0xFF, 0x7F
		);
		REQUIRE(buffer == expect);
	}

	SECTION("min number") {
		auto data {std::numeric_limits<int16_t>::min()};
		auto node = fon::binary::serialize(data);
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     11, 0, 0, 0, 0, 0, 0, 0,
		      1, 0x00, 0x80
		);
		REQUIRE(buffer == expect);
	}
}

TEST_CASE("test binary deserialization of int16_t", "[binary][int16_t]") {
	SECTION("positive number") {
		auto buffer = createVector(
		     11, 0, 0, 0, 0, 0, 0, 0,
		      1, 7, 0
		);
		auto data = fon::binary::deserialize<int16_t>(buffer);
		REQUIRE(data == 7);
	}
	SECTION("max number") {
		auto buffer = createVector(
		     11, 0, 0, 0, 0, 0, 0, 0,
		      1, 0xFF, 0x7F
		);
		auto data = fon::binary::deserialize<int16_t>(buffer);
		REQUIRE(data == std::numeric_limits<int16_t>::max());
	}
	SECTION("min number") {
		auto buffer = createVector(
		     11, 0, 0, 0, 0, 0, 0, 0,
		      1, 0x00, 0x80
		);
		auto data = fon::binary::deserialize<int16_t>(buffer);
		REQUIRE(data == std::numeric_limits<int16_t>::min());
	}
}

TEST_CASE("test binary serialization of uint16_t", "[binary][uint16_t]") {
	SECTION("small number") {
		uint16_t data {7}; // "random" number
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     11, 0, 0, 0, 0, 0, 0, 0,
		      1, 7, 0
		);
		REQUIRE(buffer == expect);
	}

	SECTION("max number") {
		auto data {std::numeric_limits<uint16_t>::max()};
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     11, 0, 0, 0, 0, 0, 0, 0,
		      1, 0xFF, 0xFF
		);
		REQUIRE(buffer == expect);
	}

	SECTION("min number") {
		auto data {std::numeric_limits<uint16_t>::min()};
		auto node = fon::binary::serialize(data);
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     11, 0, 0, 0, 0, 0, 0, 0,
		      1, 0x00, 0x00
		);
		REQUIRE(buffer == expect);
	}
}

TEST_CASE("test binary deserialization of uint16_t", "[binary][uint16_t]") {
	SECTION("positive number") {
		auto buffer = createVector(
		     11, 0, 0, 0, 0, 0, 0, 0,
		      1, 7, 0
		);
		auto data = fon::binary::deserialize<uint16_t>(buffer);
		REQUIRE(data == 7);
	}
	SECTION("max number") {
		auto buffer = createVector(
		     11, 0, 0, 0, 0, 0, 0, 0,
		      1, 0xFF, 0xFF
		);
		auto data = fon::binary::deserialize<uint16_t>(buffer);
		REQUIRE(data == std::numeric_limits<uint16_t>::max());
	}
	SECTION("min number") {
		auto buffer = createVector(
		     11, 0, 0, 0, 0, 0, 0, 0,
		      1, 0x00, 0x00
		);
		auto data = fon::binary::deserialize<uint16_t>(buffer);
		REQUIRE(data == std::numeric_limits<uint16_t>::min());
	}
}

TEST_CASE("test binary serialization of int32_t", "[binary][int32_t]") {
	SECTION("small number") {
		int32_t data {7}; // "random" number
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, 7, 0, 0, 0
		);
		REQUIRE(buffer == expect);
	}

	SECTION("max number") {
		auto data {std::numeric_limits<int32_t>::max()};
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, 0xFF, 0xFF, 0xFF, 0x7F
		);
		REQUIRE(buffer == expect);
	}

	SECTION("min number") {
		auto data {std::numeric_limits<int32_t>::min()};
		auto node = fon::binary::serialize(data);
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, 0x00, 0x00, 0x00, 0x80
		);
		REQUIRE(buffer == expect);
	}
}

TEST_CASE("test binary deserialization of int32_t", "[binary][int32_t]") {
	SECTION("positive number") {
		auto buffer = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, 7, 0, 0, 0
		);
		auto data = fon::binary::deserialize<int32_t>(buffer);
		REQUIRE(data == 7);
	}
	SECTION("max number") {
		auto buffer = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, 0xFF, 0xFF, 0xFF, 0x7F
		);
		auto data = fon::binary::deserialize<int32_t>(buffer);
		REQUIRE(data == std::numeric_limits<int32_t>::max());
	}
	SECTION("min number") {
		auto buffer = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, 0x00, 0x00, 0x00, 0x80
		);
		auto data = fon::binary::deserialize<int32_t>(buffer);
		REQUIRE(data == std::numeric_limits<int32_t>::min());
	}
}

TEST_CASE("test binary serialization of uint32_t", "[binary][uint32_t]") {
	SECTION("small number") {
		uint32_t data {7}; // "random" number
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, 7, 0, 0, 0
		);
		REQUIRE(buffer == expect);
	}

	SECTION("max number") {
		auto data {std::numeric_limits<uint32_t>::max()};
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, 0xFF, 0xFF, 0xFF, 0xFF
		);
		REQUIRE(buffer == expect);
	}

	SECTION("min number") {
		auto data {std::numeric_limits<uint32_t>::min()};
		auto node = fon::binary::serialize(data);
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, 0x00, 0x00, 0x00, 0x00
		);
		REQUIRE(buffer == expect);
	}
}

TEST_CASE("test binary deserialization of uint32_t", "[binary][uint32_t]") {
	SECTION("positive number") {
		auto buffer = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, 7, 0, 0, 0
		);
		auto data = fon::binary::deserialize<uint32_t>(buffer);
		REQUIRE(data == 7);
	}
	SECTION("max number") {
		auto buffer = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, 0xFF, 0xFF, 0xFF, 0xFF
		);
		auto data = fon::binary::deserialize<uint32_t>(buffer);
		REQUIRE(data == std::numeric_limits<uint32_t>::max());
	}
	SECTION("min number") {
		auto buffer = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1, 0x00, 0x00, 0x00, 0x00
		);
		auto data = fon::binary::deserialize<uint32_t>(buffer);
		REQUIRE(data == std::numeric_limits<uint32_t>::min());
	}
}

TEST_CASE("test binary serialization of int64_t", "[binary][int64_t]") {
	SECTION("small number") {
		int64_t data {7}; // "random" number
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1, 7, 0, 0, 0, 0, 0, 0, 0
		);
		REQUIRE(buffer == expect);
	}

	SECTION("max number") {
		auto data {std::numeric_limits<int64_t>::max()};
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F
		);
		REQUIRE(buffer == expect);
	}

	SECTION("min number") {
		auto data {std::numeric_limits<int64_t>::min()};
		auto node = fon::binary::serialize(data);
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80
		);
		REQUIRE(buffer == expect);
	}
}

TEST_CASE("test binary deserialization of int64_t", "[binary][int64_t]") {
	SECTION("positive number") {
		auto buffer = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1, 7, 0, 0, 0, 0, 0, 0, 0
		);
		auto data = fon::binary::deserialize<int64_t>(buffer);
		REQUIRE(data == 7);
	}
	SECTION("max number") {
		auto buffer = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x7F
		);
		auto data = fon::binary::deserialize<int64_t>(buffer);
		REQUIRE(data == std::numeric_limits<int64_t>::max());
	}
	SECTION("min number") {
		auto buffer = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80
		);
		auto data = fon::binary::deserialize<int64_t>(buffer);
		REQUIRE(data == std::numeric_limits<int64_t>::min());
	}
}

TEST_CASE("test binary serialization of uint64_t", "[binary][uint64_t]") {
	SECTION("small number") {
		uint64_t data {7}; // "random" number
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1, 7, 0, 0, 0, 0, 0, 0, 0
		);
		REQUIRE(buffer == expect);
	}

	SECTION("max number") {
		auto data {std::numeric_limits<uint64_t>::max()};
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		);
		REQUIRE(buffer == expect);
	}

	SECTION("min number") {
		auto data {std::numeric_limits<uint64_t>::min()};
		auto node = fon::binary::serialize(data);
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		);
		REQUIRE(buffer == expect);
	}
}

TEST_CASE("test binary deserialization of uint64_t", "[binary][uint64_t]") {
	SECTION("positive number") {
		auto buffer = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1, 7, 0, 0, 0, 0, 0, 0, 0
		);
		auto data = fon::binary::deserialize<uint64_t>(buffer);
		REQUIRE(data == 7);
	}
	SECTION("max number") {
		auto buffer = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
		);
		auto data = fon::binary::deserialize<uint64_t>(buffer);
		REQUIRE(data == std::numeric_limits<uint64_t>::max());
	}
	SECTION("min number") {
		auto buffer = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		);
		auto data = fon::binary::deserialize<uint64_t>(buffer);
		REQUIRE(data == std::numeric_limits<uint64_t>::min());
	}
}

}
