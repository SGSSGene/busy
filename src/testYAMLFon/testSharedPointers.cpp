#include <catch/catch.hpp>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>

namespace {

struct B {
	std::shared_ptr<int32_t> sptr1;
	std::shared_ptr<int32_t> sptr2;
	int* ptr{nullptr};

	template <typename Node>
	void serialize(Node& node) {
		node["sptr1"] % sptr1;
		node["sptr2"] % sptr2;
		node["ptr"]   % ptr;
	}
};

TEST_CASE("test yaml serialization of shared_ptr pointers", "[yaml][std][shared_ptr][pointer][serialize]") {
	auto data = B{};
	data.sptr1 = std::make_shared<int32_t>(42);
	data.sptr2 = data.sptr1;
	data.ptr = data.sptr1.get();

	auto node = fon::yaml::serialize(data);
	REQUIRE(node.IsMap());
	CHECK(node["sptr1"]["type"].as<std::string>() == "primary");
	CHECK(node["sptr1"]["data"].as<int32_t>() == 42);
	CHECK(node["sptr2"]["type"].as<std::string>() == "secondary");
	CHECK(node["sptr2"]["path"].as<std::string>() == "/sptr1");
	CHECK(node["ptr"].as<std::string>() == "/sptr1");
}

TEST_CASE("test yaml deserialization of shared_ptr pointers", "[yaml][std][shared_ptr][pointer][deserialize]") {
	YAML::Node node;
	node["sptr1"]["type"] = "primary";
	node["sptr1"]["data"] = 43;

	node["sptr2"]["type"] = "secondary";
	node["sptr2"]["path"] = "/sptr1";

	node["ptr"] = "/sptr1";

	auto data = fon::yaml::deserialize<B>(node);
	REQUIRE(data.sptr1);
	REQUIRE(data.sptr2);
	CHECK(data.sptr1.get() == data.sptr2.get());
	CHECK(data.ptr == data.sptr1.get());
	CHECK(*data.sptr1 == 43);
}

}
