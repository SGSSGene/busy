#include <catch/catch.hpp>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>

#include <fmt/format.h>

namespace {

struct B {
    YAML::Node node;

    template <typename Node, typename Self>
    static void reflect(Node& _node, Self& self) {
        _node["node"] % self.node;
    }
};

TEST_CASE("test yaml serialization of YAML::Node", "[yaml][serialize][yaml-cpp]") {
    auto data = B{};
    data.node["foo"] = "bar";

    auto node = fon::yaml::serialize(data);

    REQUIRE(node.IsMap());
    REQUIRE(node["node"].IsMap());
    CHECK(node["node"]["foo"].as<std::string>() == "bar");
}

TEST_CASE("test yaml deserialization of YAML::Node", "[yaml][deserialize][yaml-cpp]") {
    YAML::Node node;
    node["node"]["foo"] = "bar";

    auto data = fon::yaml::deserialize<B>(node);
    REQUIRE(data.node.IsMap());
    CHECK(data.node["foo"].as<std::string>() == "bar");
}

}
