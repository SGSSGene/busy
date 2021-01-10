#include <catch/catch.hpp>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>

TEST_CASE("test yaml serialization of float", "[yaml][float]") {
	SECTION("positive number") {
		float data {1.34}; // "random" number
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<float>() == 1.34f);
	}

	SECTION("small number") {
		float data {-0.45}; // "random" number
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<float>() == -0.45f);
	}
}

TEST_CASE("test yaml deserialization of float", "[yaml][float]") {
	SECTION("positive number") {
		YAML::Node node;
		node = 1.34; // "random" number
		auto data = fon::yaml::deserialize<float>(node);
		REQUIRE(data == 1.34f);
	}
	SECTION("small number") {
		YAML::Node node;
		node = -0.45; // "random" number
		auto data = fon::yaml::deserialize<float>(node);
		REQUIRE(data == -0.45f);
	}
}
TEST_CASE("test yaml serialization of double", "[yaml][double]") {
	SECTION("positive number") {
		double data {1.34}; // "random" number
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<double>() == 1.34);
	}

	SECTION("small number") {
		double data {-0.45}; // "random" number
		auto node = fon::yaml::serialize(data);
		REQUIRE(node.IsScalar());
		REQUIRE(node.as<double>() == -0.45);
	}
}

TEST_CASE("test yaml deserialization of double", "[yaml][double]") {
	SECTION("positive number") {
		YAML::Node node;
		node = 1.34; // "random" number
		auto data = fon::yaml::deserialize<double>(node);
		REQUIRE(data == 1.34);
	}
	SECTION("small number") {
		YAML::Node node;
		node = -0.45; // "random" number
		auto data = fon::yaml::deserialize<double>(node);
		REQUIRE(data == -0.45);
	}
}
