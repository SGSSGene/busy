#include <catch/catch.hpp>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>

TEST_CASE("test yaml serialization of int8_t", "[yaml][int8_t]") {
	SECTION("positive number") {
		int8_t data {7}; // "random" number
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<int8_t>() == 7);
	}

	SECTION("small number") {
		int8_t data {-7}; // "random" number
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<int8_t>() == -7);
	}

	SECTION("max number") {
		int8_t data {127};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<int8_t>() == 127);
	}

	SECTION("min number") {
		int8_t data {-128};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<int8_t>() == -128);
	}
}

TEST_CASE("test yaml deserialization of int8_t", "[yaml][int8_t]") {
	SECTION("positive number") {
		YAML::Node node;
		node = 7; // "random" number
		auto data = fon::yaml::deserialize<int8_t>(node);
		REQUIRE(data == 7);
	}
	SECTION("small number") {
		YAML::Node node;
		node = -7; // "random" number
		auto data = fon::yaml::deserialize<int8_t>(node);
		REQUIRE(data == -7);
	}
	SECTION("max number") {
		YAML::Node node;
		node = 127;
		auto data = fon::yaml::deserialize<int8_t>(node);
		REQUIRE(data == 127);
	}
	SECTION("min number") {
		YAML::Node node;
		node = -128;
		auto data = fon::yaml::deserialize<int8_t>(node);
		REQUIRE(data == -128);
	}

	SECTION("number above range") {
		YAML::Node node;
		node = 128;
		REQUIRE_THROWS(fon::yaml::deserialize<int8_t>(node));
	}
	SECTION("number below range") {
		YAML::Node node;
		node = -129;
		REQUIRE_THROWS(fon::yaml::deserialize<int8_t>(node));
	}
}

TEST_CASE("test yaml serialization of uint8_t", "[yaml][uint8_t]") {
	SECTION("positive number") {
		uint8_t data {7}; // "random" number
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<uint8_t>() == 7);
	}

	SECTION("max number") {
		uint8_t data {255};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<uint8_t>() == 255);
	}

	SECTION("min number") {
		uint8_t data {0};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<uint8_t>() == 0);
	}
}

TEST_CASE("test yaml deserialization of uint8_t", "[yaml][uint8_t]") {
	SECTION("positive number") {
		YAML::Node node;
		node = 7; // "random" number
		auto data = fon::yaml::deserialize<uint8_t>(node);
		REQUIRE(data == 7);
	}
	SECTION("max number") {
		YAML::Node node;
		node = 255;
		auto data = fon::yaml::deserialize<uint8_t>(node);
		REQUIRE(data == 255);
	}
	SECTION("min number") {
		YAML::Node node;
		node = 0;
		auto data = fon::yaml::deserialize<uint8_t>(node);
		REQUIRE(data == 0);
	}

	SECTION("number above range") {
		YAML::Node node;
		node = 256;
		REQUIRE_THROWS(fon::yaml::deserialize<uint8_t>(node));
	}
	SECTION("number below range") {
		YAML::Node node;
		node = -1;
		REQUIRE_THROWS(fon::yaml::deserialize<uint8_t>(node));
	}
}

TEST_CASE("test yaml serialization of int16_t", "[yaml][int16_t]") {
	SECTION("positive number") {
		int16_t data {7}; // "random" number
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<int16_t>() == 7);
	}

	SECTION("max number") {
		int16_t data {std::numeric_limits<int16_t>::max()};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<int16_t>() == std::numeric_limits<int16_t>::max());
	}

	SECTION("min number") {
		int16_t data {std::numeric_limits<int16_t>::min()};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<int16_t>() == std::numeric_limits<int16_t>::min());
	}
}

TEST_CASE("test yaml deserialization of int16_t", "[yaml][int16_t]") {
	SECTION("positive number") {
		YAML::Node node;
		node = 7; // "random" number
		auto data = fon::yaml::deserialize<int16_t>(node);
		REQUIRE(data == 7);
	}
	SECTION("max number") {
		YAML::Node node;
		node = std::numeric_limits<int16_t>::max();
		auto data = fon::yaml::deserialize<int16_t>(node);
		REQUIRE(data == std::numeric_limits<int16_t>::max());
	}
	SECTION("min number") {
		YAML::Node node;
		node = std::numeric_limits<int16_t>::min();
		auto data = fon::yaml::deserialize<int16_t>(node);
		REQUIRE(data == std::numeric_limits<int16_t>::min());
	}

	SECTION("number above range") {
		YAML::Node node;
		node = std::numeric_limits<int16_t>::max() + 1;
		REQUIRE_THROWS(fon::yaml::deserialize<int16_t>(node));
	}
	SECTION("number below range") {
		YAML::Node node;
		node = std::numeric_limits<int16_t>::min() - 1;
		REQUIRE_THROWS(fon::yaml::deserialize<int16_t>(node));
	}
}

TEST_CASE("test yaml serialization of uint16_t", "[yaml][uint16_t]") {
	SECTION("positive number") {
		uint16_t data {7}; // "random" number
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<uint16_t>() == 7);
	}

	SECTION("max number") {
		uint16_t data {std::numeric_limits<uint16_t>::max()};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<uint16_t>() == std::numeric_limits<uint16_t>::max());
	}

	SECTION("min number") {
		uint16_t data {std::numeric_limits<uint16_t>::min()};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<uint16_t>() == std::numeric_limits<uint16_t>::min());
	}
}

TEST_CASE("test yaml deserialization of uint16_t", "[yaml][uint16_t]") {
	SECTION("positive number") {
		YAML::Node node;
		node = 7; // "random" number
		auto data = fon::yaml::deserialize<uint16_t>(node);
		REQUIRE(data == 7);
	}
	SECTION("max number") {
		YAML::Node node;
		node = std::numeric_limits<uint16_t>::max();
		auto data = fon::yaml::deserialize<uint16_t>(node);
		REQUIRE(data == std::numeric_limits<uint16_t>::max());
	}
	SECTION("min number") {
		YAML::Node node;
		node = std::numeric_limits<uint16_t>::min();
		auto data = fon::yaml::deserialize<uint16_t>(node);
		REQUIRE(data == std::numeric_limits<uint16_t>::min());
	}

	SECTION("number above range") {
		YAML::Node node;
		node = std::numeric_limits<uint16_t>::max() + 1;
		REQUIRE_THROWS(fon::yaml::deserialize<uint16_t>(node));
	}
	SECTION("number below range") {
		YAML::Node node;
		node = -1;
//		auto data = fon::yaml::deserialize<uint16_t>(node);
//		std::cout << data << "\n";
		REQUIRE_THROWS(fon::yaml::deserialize<uint16_t>(node));
	}
}
TEST_CASE("test yaml serialization of int32_t", "[yaml][int32_t]") {
	SECTION("positive number") {
		int32_t data {7}; // "random" number
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<int32_t>() == 7);
	}

	SECTION("max number") {
		int32_t data {std::numeric_limits<int32_t>::max()};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<int32_t>() == std::numeric_limits<int32_t>::max());
	}

	SECTION("min number") {
		int32_t data {std::numeric_limits<int32_t>::min()};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<int32_t>() == std::numeric_limits<int32_t>::min());
	}
}

TEST_CASE("test yaml deserialization of int32_t", "[yaml][int32_t]") {
	SECTION("positive number") {
		YAML::Node node;
		node = 7; // "random" number
		auto data = fon::yaml::deserialize<int32_t>(node);
		REQUIRE(data == 7);
	}
	SECTION("max number") {
		YAML::Node node;
		node = std::numeric_limits<int32_t>::max();
		auto data = fon::yaml::deserialize<int32_t>(node);
		REQUIRE(data == std::numeric_limits<int32_t>::max());
	}
	SECTION("min number") {
		YAML::Node node;
		node = std::numeric_limits<int32_t>::min();
		auto data = fon::yaml::deserialize<int32_t>(node);
		REQUIRE(data == std::numeric_limits<int32_t>::min());
	}

	SECTION("number above range") {
		YAML::Node node;
		node = std::numeric_limits<int32_t>::max() + 1ll;
		REQUIRE_THROWS(fon::yaml::deserialize<int32_t>(node));
	}
	SECTION("number below range") {
		YAML::Node node;
		node = std::numeric_limits<int32_t>::min() - 1ll;
		REQUIRE_THROWS(fon::yaml::deserialize<int32_t>(node));
	}
}

TEST_CASE("test yaml serialization of uint32_t", "[yaml][uint32_t]") {
	SECTION("positive number") {
		uint32_t data {7}; // "random" number
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<uint32_t>() == 7);
	}

	SECTION("max number") {
		uint32_t data {std::numeric_limits<uint32_t>::max()};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<uint32_t>() == std::numeric_limits<uint32_t>::max());
	}

	SECTION("min number") {
		uint32_t data {std::numeric_limits<uint32_t>::min()};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<uint32_t>() == std::numeric_limits<uint32_t>::min());
	}
}

TEST_CASE("test yaml deserialization of uint32_t", "[yaml][uint32_t]") {
	SECTION("positive number") {
		YAML::Node node;
		node = 7; // "random" number
		auto data = fon::yaml::deserialize<uint32_t>(node);
		REQUIRE(data == 7);
	}
	SECTION("max number") {
		YAML::Node node;
		node = std::numeric_limits<uint32_t>::max();
		auto data = fon::yaml::deserialize<uint32_t>(node);
		REQUIRE(data == std::numeric_limits<uint32_t>::max());
	}
	SECTION("min number") {
		YAML::Node node;
		node = std::numeric_limits<uint32_t>::min();
		auto data = fon::yaml::deserialize<uint32_t>(node);
		REQUIRE(data == std::numeric_limits<uint32_t>::min());
	}

	SECTION("number above range") {
		YAML::Node node;
		node = std::numeric_limits<uint32_t>::max() + 1ll;
		REQUIRE_THROWS(fon::yaml::deserialize<uint32_t>(node));
	}
	SECTION("number below range") {
		YAML::Node node;
		node = int64_t{std::numeric_limits<uint32_t>::min()} - 1ll;
		REQUIRE_THROWS(fon::yaml::deserialize<uint32_t>(node));
	}
}
TEST_CASE("test yaml serialization of int64_t", "[yaml][int64_t]") {
	SECTION("positive number") {
		int64_t data {7}; // "random" number
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<int64_t>() == 7);
	}

	SECTION("max number") {
		int64_t data {std::numeric_limits<int64_t>::max()};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<int64_t>() == std::numeric_limits<int64_t>::max());
	}

	SECTION("min number") {
		int64_t data {std::numeric_limits<int64_t>::min()};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<int64_t>() == std::numeric_limits<int64_t>::min());
	}
}

TEST_CASE("test yaml deserialization of int64_t", "[yaml][int64_t]") {
	SECTION("positive number") {
		YAML::Node node;
		node = 7; // "random" number
		auto data = fon::yaml::deserialize<int64_t>(node);
		REQUIRE(data == 7);
	}
	SECTION("max number") {
		YAML::Node node;
		node = std::numeric_limits<int64_t>::max();
		auto data = fon::yaml::deserialize<int64_t>(node);
		REQUIRE(data == std::numeric_limits<int64_t>::max());
	}
	SECTION("min number") {
		YAML::Node node;
		node = std::numeric_limits<int64_t>::min();
		auto data = fon::yaml::deserialize<int64_t>(node);
		REQUIRE(data == std::numeric_limits<int64_t>::min());
	}
}

TEST_CASE("test yaml serialization of uint64_t", "[yaml][uint64_t]") {
	SECTION("positive number") {
		uint64_t data {7}; // "random" number
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<uint64_t>() == 7);
	}

	SECTION("max number") {
		uint64_t data {std::numeric_limits<uint64_t>::max()};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<uint64_t>() == std::numeric_limits<uint64_t>::max());
	}

	SECTION("min number") {
		uint64_t data {std::numeric_limits<uint64_t>::min()};
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<uint64_t>() == std::numeric_limits<uint64_t>::min());
	}
}

TEST_CASE("test yaml deserialization of uint64_t", "[yaml][uint64_t]") {
	SECTION("positive number") {
		YAML::Node node;
		node = 7; // "random" number
		auto data = fon::yaml::deserialize<uint64_t>(node);
		REQUIRE(data == 7);
	}
	SECTION("max number") {
		YAML::Node node;
		node = std::numeric_limits<uint64_t>::max();
		auto data = fon::yaml::deserialize<uint64_t>(node);
		REQUIRE(data == std::numeric_limits<uint64_t>::max());
	}
	SECTION("min number") {
		YAML::Node node;
		node = std::numeric_limits<uint64_t>::min();
		auto data = fon::yaml::deserialize<uint64_t>(node);
		REQUIRE(data == std::numeric_limits<uint64_t>::min());
	}
}
