#include <catch/catch.hpp>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>

namespace {

template <typename ...Ts>
auto createVector(Ts... ts) {
	return std::vector<std::byte>{std::byte{uint8_t(ts)}...};
}

template <typename T>
auto asBinary(T const& obj) {
	auto data = std::vector<std::byte>(sizeof(T));
	std::memcpy(data.data(), &obj, data.size());
	return data;
}

TEST_CASE("test binary serialization of float", "[binary][float]") {
	SECTION("positive number") {
		float data {1.34}; // "random" number
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1
		);
		for (auto b : asBinary(data)) {
			expect.push_back(b);
		}
		REQUIRE(buffer == expect);
	}

	SECTION("small number") {
		float data {-0.45}; // "random" number
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1
		);
		for (auto b : asBinary(data)) {
			expect.push_back(b);
		}
		REQUIRE(buffer == expect);
	}
}

TEST_CASE("test binary deserialization of float", "[binary][float]") {
	SECTION("positive number") {
		auto buffer = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1
		);
		for (auto b : asBinary(float{1.34})) {
			buffer.push_back(b);
		}
		auto data = fon::binary::deserialize<float>(buffer);
		REQUIRE(data == 1.34f);
	}
	SECTION("small number") {
		auto buffer = createVector(
		     13, 0, 0, 0, 0, 0, 0, 0,
		      1
		);
		for (auto b : asBinary(float{-0.45})) {
			buffer.push_back(b);
		}
		auto data = fon::binary::deserialize<float>(buffer);
		REQUIRE(data == -0.45f);
	}
}

TEST_CASE("test binary serialization of double", "[binary][double]") {
	SECTION("positive number") {
		double data {1.34}; // "random" number
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1
		);
		for (auto b : asBinary(data)) {
			expect.push_back(b);
		}
		REQUIRE(buffer == expect);
	}

	SECTION("small number") {
		double data {-0.45}; // "random" number
		auto buffer = fon::binary::serialize(data);
		auto expect = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1
		);
		for (auto b : asBinary(data)) {
			expect.push_back(b);
		}
		REQUIRE(buffer == expect);
	}
}

TEST_CASE("test binary deserialization of double", "[binary][double]") {
	SECTION("positive number") {
		auto buffer = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1
		);
		for (auto b : asBinary(double{1.34})) {
			buffer.push_back(b);
		}
		auto data = fon::binary::deserialize<double>(buffer);
		REQUIRE(data == 1.34);
	}
	SECTION("small number") {
		auto buffer = createVector(
		     17, 0, 0, 0, 0, 0, 0, 0,
		      1
		);
		for (auto b : asBinary(double{-0.45})) {
			buffer.push_back(b);
		}
		auto data = fon::binary::deserialize<double>(buffer);
		REQUIRE(data == -0.45);
	}
}
}
