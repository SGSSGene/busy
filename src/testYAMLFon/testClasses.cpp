#include <catch/catch.hpp>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>

namespace {
struct A {
    std::vector<int32_t> xs;

    template <typename Node, typename Self>
    constexpr static void reflect(Node& node, Self& self) {
        node["xs"] % self.xs;
    }
};

auto asString(YAML::Node node) -> std::string {
    YAML::Emitter emit;
    emit << node;
    return emit.c_str();
}
}


TEST_CASE("test yaml serialization of struct A", "[yaml][struct][serialize]") {
    auto data = A{{10, 20, 30}};
    auto node = fon::yaml::serialize(data);

    INFO(asString(node));

    REQUIRE(node.IsMap());
    REQUIRE(node["xs"].IsSequence());
    REQUIRE(node["xs"].size() == 3);
    REQUIRE(node["xs"][0].as<int32_t>() == 10);
    REQUIRE(node["xs"][1].as<int32_t>() == 20);
    REQUIRE(node["xs"][2].as<int32_t>() == 30);
}
TEST_CASE("test yaml deserialization of struct A", "[yaml][struct][deserialize]") {
    YAML::Node node;
    node["xs"][0] = 10;
    node["xs"][1] = 20;
    node["xs"][2] = 30;
    auto data = fon::yaml::deserialize<A>(node);
    REQUIRE(data.xs.size() == 3);
    REQUIRE(data.xs == (std::vector<int32_t>{10, 20, 30}));
}

struct B {
    struct C {
        int32_t x;

        template <typename Node, typename Self>
        static void reflect(Node& node, Self& self) {
            node["x"] % self.x;
        }
    };
    std::map<std::string, C> infos;

    template <typename Node, typename Self>
    static void reflect(Node& node, Self& self) {
        node["infos"] % self.infos;
    }
};

TEST_CASE("test yaml serialization of maps with structs", "[yaml][struct][std][map][serialize]") {
    auto data = B{};
    data.infos["k1"].x = 10;
    data.infos["k2"].x = 20;

    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsMap());
    REQUIRE(node.size() == 1);
    REQUIRE(node["infos"].IsMap());
    REQUIRE(node["infos"].size() == 2);
    REQUIRE(node["infos"]["k1"]["x"].as<int>() == 10);
    REQUIRE(node["infos"]["k2"]["x"].as<int>() == 20);
}

TEST_CASE("test yaml deserialization of maps with structs", "[yaml][struct][std][map][deserialize]") {
    YAML::Node node;
    node["infos"]["k1"]["x"] = 10;
    node["infos"]["k2"]["x"] = 20;

    REQUIRE(node.IsMap());
    REQUIRE(node["infos"].IsMap());
    REQUIRE(node["infos"]["k1"].IsMap());
    REQUIRE(node["infos"]["k2"].IsMap());
    REQUIRE(node["infos"]["k1"]["x"].IsScalar());
    REQUIRE(node["infos"]["k2"]["x"].IsScalar());



    auto data = fon::yaml::deserialize<B>(node);

    REQUIRE(data.infos.size() == 2);
    REQUIRE(data.infos.find("k1") != data.infos.end());
    REQUIRE(data.infos.find("k2") != data.infos.end());
    REQUIRE(data.infos.at("k1").x == 10);
    REQUIRE(data.infos.at("k2").x == 20);
}



struct D {
    std::vector<int32_t> xs;

    template <typename Node, typename Self>
    static void reflect(Node& node, Self& self) {
        node % self.xs;
    }
};


TEST_CASE("test yaml serialization of struct D (no name)", "[yaml][struct][serialize][direct]") {
    auto data = D{{10, 20, 30}};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsSequence());
    REQUIRE(node.size() == 3);
    REQUIRE(node[0].as<int32_t>() == 10);
    REQUIRE(node[1].as<int32_t>() == 20);
    REQUIRE(node[2].as<int32_t>() == 30);
}

TEST_CASE("test yaml deserialization of struct D (no name)", "[yaml][struct][deserialize][direct]") {
    YAML::Node node;
    node[0] = 10;
    node[1] = 20;
    node[2] = 30;
    auto data = fon::yaml::deserialize<D>(node);
    REQUIRE(data.xs == (std::vector<int32_t>{10, 20, 30}));
}
