#include <catch/catch.hpp>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>

namespace {

struct A {
	std::vector<int32_t> xs;
	int* ptr{nullptr};

	template <typename Node>
	void serialize(Node& node) {
		node["xs"] % xs;
		node["ptr"] % ptr;
	}
};


TEST_CASE("test yaml serialization of raw pointers", "[yaml][raw][pointer][serialize]") {
	auto data = A{{10, 20, 30}};
	data.ptr = &data.xs[1];

	auto node = fon::yaml::serialize(data);
	REQUIRE(node.IsMap());
	REQUIRE(node["ptr"].as<std::string>() == "/xs/1");
}

TEST_CASE("test yaml deserialization of raw pointers", "[yaml][raw][pointer][deserialize]") {
	YAML::Node node;
	node["xs"][0] = 10;
	node["xs"][1] = 20;
	node["xs"][2] = 30;
	node["ptr"] = "/xs/1";

	auto data = fon::yaml::deserialize<A>(node);
	REQUIRE(data.xs == (std::vector<int32_t>{10, 20, 30}));
	REQUIRE(data.ptr == &data.xs[1]);
}

struct B {
	std::unique_ptr<int32_t> uptr;
	int* ptr{nullptr};

	template <typename Node>
	void serialize(Node& node) {
		node["uptr"] % uptr;
		node["ptr"] % ptr;
	}
};

TEST_CASE("test yaml serialization of unique_ptr pointers", "[yaml][std][unique_ptr][pointer][serialize]") {
	auto data = B{};
	data.uptr = std::make_unique<int32_t>(42);
	data.ptr = data.uptr.get();

	auto node = fon::yaml::serialize(data);
	REQUIRE(node.IsMap());
	CHECK(node["uptr"].as<int32_t>() == 42);
	CHECK(node["ptr"].as<std::string>() == "/uptr");
}

TEST_CASE("test yaml deserialization of unique_ptr pointers", "[yaml][std][unique_ptr][pointer][deserialize]") {
	YAML::Node node;
	node["uptr"] = 43;
	node["ptr"] = "/uptr";

	auto data = fon::yaml::deserialize<B>(node);
	REQUIRE(data.uptr);
	REQUIRE(*data.uptr == 43);
	REQUIRE(data.ptr == data.uptr.get());
}

struct Base {
	int32_t x;
	template <typename Node>
	void serialize(Node& node) {
		node["x"] % x;
	}
};
struct Derived : Base {
	int32_t y;
	Base* self;

	template <typename Node>
	void serialize(Node& node) {
		Base::serialize(node);
		node["y"] % y;
		node["self"] % self;
	}
};

TEST_CASE("test yaml serialization of base pointers", "[yaml][raw][base][pointer][serialize]") {
	auto data = std::make_unique<Derived>();
	data->x = 5;
	data->y = 6;
	data->self = data.get();

	auto node = fon::yaml::serialize(data);
	REQUIRE(node.IsMap());
	CHECK(node["x"].as<int32_t>() == 5);
	CHECK(node["y"].as<int32_t>() == 6);
	CHECK(node["self"].as<std::string>() == "");
}

TEST_CASE("test yaml deserialization of base pointers", "[yaml][raw][base][pointer][deserialize]") {
	YAML::Node node;
	node["x"] = 7;
	node["y"] = 8;
	node["self"] = "";

	auto data = fon::yaml::deserialize<std::unique_ptr<Derived>>(node);
	CHECK(data->x == 7);
	CHECK(data->y == 8);
	CHECK(data->self == data.get());
}

}
