#include <catch/catch.hpp>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>
#include <flattensObjectsNeatly/filesystem.h>
#include <flattensObjectsNeatly/chrono.h>


TEST_CASE("test yaml serialization of std::string", "[yaml][std][string][serialize]") {
    auto data = std::string{"hallo welt"};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsScalar());
    REQUIRE(node.as<std::string>() == "hallo welt");
}
TEST_CASE("test yaml deserialization of std::string", "[yaml][std][string][deserialize]") {
    YAML::Node node;
    node = "hallo welt";
    auto data = fon::yaml::deserialize<std::string>(node);
    REQUIRE(data == "hallo welt");
}

TEST_CASE("test yaml serialization of std::vector", "[yaml][std][vector][serialize]") {
    auto data = std::vector<int32_t>{10, 20, 30};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsSequence());
    REQUIRE(node.size() == 3);
    REQUIRE(node[0].as<int32_t>() == 10);
    REQUIRE(node[1].as<int32_t>() == 20);
    REQUIRE(node[2].as<int32_t>() == 30);
}

TEST_CASE("test yaml deserialization of std::vector", "[yaml][std][vector][deserialize]") {
    YAML::Node node;
    node[0] = 10;
    node[1] = 20;
    node[2] = 30;
    auto data = fon::yaml::deserialize<std::vector<int32_t>>(node);
    REQUIRE(data == (std::vector<int32_t>{10, 20, 30}));
}

TEST_CASE("test yaml serialization of std::array", "[yaml][std][array][serialize]") {
    auto data = std::array<int32_t, 3>{10, 20, 30};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsMap());
    REQUIRE(node.size() == 3);
    REQUIRE(node[0].as<int32_t>() == 10);
    REQUIRE(node[1].as<int32_t>() == 20);
    REQUIRE(node[2].as<int32_t>() == 30);
}

TEST_CASE("test yaml deserialization of std::array", "[yaml][std][array][deserialize]") {
    YAML::Node node;
    node[0] = 10;
    node[1] = 20;
    node[2] = 30;
    auto data = fon::yaml::deserialize<std::array<int32_t, 3>>(node);
    REQUIRE(data == (std::array<int32_t, 3>{10, 20, 30}));
}

TEST_CASE("test yaml serialization of std::list", "[yaml][std][list][serialize]") {
    auto data = std::list<int32_t>{10, 20, 30};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsSequence());
    REQUIRE(node.size() == 3);
    REQUIRE(node[0].as<int32_t>() == 10);
    REQUIRE(node[1].as<int32_t>() == 20);
    REQUIRE(node[2].as<int32_t>() == 30);
}
TEST_CASE("test yaml deserialization of std::list", "[yaml][std][list][deserialize]") {
    YAML::Node node;
    node[0] = 10;
    node[1] = 20;
    node[2] = 30;
    auto data = fon::yaml::deserialize<std::list<int32_t>>(node);
    REQUIRE(data == (std::list<int32_t>{10, 20, 30}));
}
#if 0
TEST_CASE("test yaml serialization of std::forward_list", "[yaml][std][forward_list][serialize]") {
    auto data = std::forward_list<int32_t>{10, 20, 30};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsSequence());
    REQUIRE(node.size() == 3);
    REQUIRE(node[0].as<int32_t>() == 10);
    REQUIRE(node[1].as<int32_t>() == 20);
    REQUIRE(node[2].as<int32_t>() == 30);
}
TEST_CASE("test yaml deserialization of std::forward_list", "[yaml][std][forward_list][deserialize]") {
    YAML::Node node;
    node[0] = 10;
    node[1] = 20;
    node[2] = 30;
    auto data = fon::yaml::deserialize<std::forward_list<int32_t>>(node);
    REQUIRE(data == (std::forward_list<int32_t>{10, 20, 30}));
}
#endif

TEST_CASE("test yaml serialization of std::deque", "[yaml][std][deque][serialize]") {
    auto data = std::deque<int32_t>{10, 20, 30};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsSequence());
    REQUIRE(node.size() == 3);
    REQUIRE(node[0].as<int32_t>() == 10);
    REQUIRE(node[1].as<int32_t>() == 20);
    REQUIRE(node[2].as<int32_t>() == 30);
}
TEST_CASE("test yaml deserialization of std::deque", "[yaml][std][deque][deserialize]") {
    YAML::Node node;
    node[0] = 10;
    node[1] = 20;
    node[2] = 30;
    auto data = fon::yaml::deserialize<std::deque<int32_t>>(node);
    REQUIRE(data == (std::deque<int32_t>{10, 20, 30}));
}

TEST_CASE("test yaml serialization of std::set", "[yaml][std][set][serialize]") {
    auto data = std::set<int32_t>{10, 20, 30};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsSequence());
    REQUIRE(node.size() == 3);
    REQUIRE(node[0].as<int32_t>() == 10);
    REQUIRE(node[1].as<int32_t>() == 20);
    REQUIRE(node[2].as<int32_t>() == 30);
}
TEST_CASE("test yaml deserialization of std::set", "[yaml][std][set][deserialize]") {
    YAML::Node node;
    node[0] = 10;
    node[1] = 20;
    node[2] = 30;
    auto data = fon::yaml::deserialize<std::set<int32_t>>(node);
    REQUIRE(data == (std::set<int32_t>{10, 20, 30}));
}
TEST_CASE("test yaml serialization of std::unordered_set", "[yaml][std][unordered_set][serialize]") {
    auto data = std::unordered_set<int32_t>{10, 20, 30};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsSequence());
    REQUIRE(node.size() == 3);
    REQUIRE((std::set<int32_t>{node[0].as<int32_t>(), node[1].as<int32_t>(), node[2].as<int32_t>()}) == (std::set<int32_t>{10, 20, 30}));
}
TEST_CASE("test yaml deserialization of std::unordered_set", "[yaml][std][unordered_set][deserialize]") {
    YAML::Node node;
    node[0] = 10;
    node[1] = 20;
    node[2] = 30;
    auto data = fon::yaml::deserialize<std::unordered_set<int32_t>>(node);
    REQUIRE(data.size() == 3);
    REQUIRE(data.count(10) == 1);
    REQUIRE(data.count(20) == 1);
    REQUIRE(data.count(30) == 1);
}
TEST_CASE("test yaml serialization of std::map", "[yaml][std][map][serialize]") {
    auto data = std::map<std::string, int32_t>{{"k1", 10}, {"k2", 20}, {"k3", 30}};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsMap());
    REQUIRE(node.size() == 3);
    REQUIRE(node["k1"].as<int32_t>() == 10);
    REQUIRE(node["k2"].as<int32_t>() == 20);
    REQUIRE(node["k3"].as<int32_t>() == 30);
}
TEST_CASE("test yaml deserialization of std::map", "[yaml][std][map][deserialize]") {
    YAML::Node node;
    node["k1"] = 10;
    node["k2"] = 20;
    node["k3"] = 30;
    auto data = fon::yaml::deserialize<std::map<std::string, int32_t>>(node);
    REQUIRE(data == (std::map<std::string, int32_t>{{"k1", 10}, {"k2", 20}, {"k3", 30}}));
}

TEST_CASE("test yaml serialization of std::unordered_map", "[yaml][std][unordered_map][serialize]") {
    auto data = std::unordered_map<std::string, int32_t>{{"k1", 10}, {"k2", 20}, {"k3", 30}};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsMap());
    REQUIRE(node.size() == 3);
    REQUIRE(node["k1"].as<int32_t>() == 10);
    REQUIRE(node["k2"].as<int32_t>() == 20);
    REQUIRE(node["k3"].as<int32_t>() == 30);
}
TEST_CASE("test yaml deserialization of std::unordered_map", "[yaml][std][unordered_map][deserialize]") {
    YAML::Node node;
    node["k1"] = 10;
    node["k2"] = 20;
    node["k3"] = 30;
    auto data = fon::yaml::deserialize<std::unordered_map<std::string, int32_t>>(node);
    REQUIRE(data == (std::unordered_map<std::string, int32_t>{{"k1", 10}, {"k2", 20}, {"k3", 30}}));
}

TEST_CASE("test yaml serialization of std::pair", "[yaml][std][pair][serialize]") {
    auto data = std::pair<std::string, int32_t>{"hallo welt", 42};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsMap());
    REQUIRE(node.size() == 2);
    REQUIRE(node[0].as<std::string>() == "hallo welt");
    REQUIRE(node[1].as<int32_t>() == 42);
}
TEST_CASE("test yaml deserialization of std::pair", "[yaml][std][pair][deserialize]") {
    YAML::Node node;
    node[0] = "hallo welt";
    node[1] = 42;
    auto data = fon::yaml::deserialize<std::pair<std::string, int32_t>>(node);
    REQUIRE(data.first == "hallo welt");
    REQUIRE(data.second == 42);
}

TEST_CASE("test yaml serialization of std::tuple<>", "[yaml][std][tuple][serialize]") {
    auto data = std::tuple<>{};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.size() == 0);
}
TEST_CASE("test yaml deserialization of std::tuple<>", "[yaml][std][tuple][deserialize]") {
    YAML::Node node;
    auto data = fon::yaml::deserialize<std::tuple<>>(node);
    (void)data;
}

TEST_CASE("test yaml serialization of std::tuple<X>", "[yaml][std][tuple][serialize]") {
    auto data = std::tuple<std::string> {"hallo welt"};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsMap());
    REQUIRE(node.size() == 1);
    REQUIRE(node[0].as<std::string>() == "hallo welt");
}
TEST_CASE("test yaml deserialization of std::tuple<X>", "[yaml][std][tuple][deserialize]") {
    YAML::Node node;
    node[0] = "hallo welt";
    auto data = fon::yaml::deserialize<std::tuple<std::string>>(node);
    REQUIRE(std::get<0>(data) == "hallo welt");
}


TEST_CASE("test yaml serialization of std::tuple<X, Y>", "[yaml][std][tuple][serialize]") {
    auto data = std::tuple<std::string, int32_t>{"hallo welt", 42};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsMap());
    REQUIRE(node.size() == 2);
    REQUIRE(node[0].as<std::string>() == "hallo welt");
    REQUIRE(node[1].as<int32_t>() == 42);
}
TEST_CASE("test yaml deserialization of std::tuple<X, Y>", "[yaml][std][tuple][deserialize]") {
    YAML::Node node;
    node[0] = "hallo welt";
    node[1] = 42;
    auto data = fon::yaml::deserialize<std::tuple<std::string, int32_t>>(node);
    REQUIRE(std::get<0>(data) == "hallo welt");
    REQUIRE(std::get<1>(data) == 42);
}

TEST_CASE("test yaml serialization of std::optional", "[yaml][std][optional][serialize]") {
    auto data = std::optional<std::string>{"hallo welt"};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.IsSequence());
    REQUIRE(node.size() == 1);
    REQUIRE(node[0].IsScalar());
    REQUIRE(node[0].as<std::string>() == "hallo welt");
}

TEST_CASE("test yaml deserialization of std::optional", "[yaml][std][optional][deserialize]") {
    YAML::Node node;
    node[0] = "hallo welt";
    auto data = fon::yaml::deserialize<std::optional<std::string>>(node);
    REQUIRE(data.has_value());
    REQUIRE(data.value() == "hallo welt");
}

TEST_CASE("test yaml serialization of empty std::optional", "[yaml][std][optional][serialize]") {
    auto data = std::optional<std::string>{};
    auto node = fon::yaml::serialize(data);
    REQUIRE(node.size() == 0);
}
TEST_CASE("test yaml deserialization of empty std::optional", "[yaml][std][optional][deserialize]") {
    YAML::Node node;
    auto data = fon::yaml::deserialize<std::optional<std::string>>(node);
    REQUIRE(not data.has_value());
}

TEST_CASE("test yaml serialization of std::variant (index 0)", "[yaml][std][variant][serialize]") {
    auto data = std::variant<std::string, int32_t, bool>{std::string{"hallo welt"}};
    auto node = fon::yaml::serialize(data);

    REQUIRE(node.IsMap());
    REQUIRE(node.size() == 1);

    REQUIRE(node[0].IsScalar());
    REQUIRE(node[0].as<std::string>() == "hallo welt");
}
TEST_CASE("test yaml deserialization of std::variant (index 0)", "[yaml][std][variant][deserialize]") {
    YAML::Node node {YAML::NodeType::Map};
    node[0] = "hallo welt";
    auto data = fon::yaml::deserialize<std::variant<std::string, int32_t, bool>>(node);
    REQUIRE(data.index() == 0);
    REQUIRE(std::get<0>(data) == "hallo welt");
}

TEST_CASE("test yaml serialization of std::variant (index 1)", "[yaml][std][variant][serialize]") {
    auto data = std::variant<std::string, int32_t, bool>{int32_t{42}};
    auto node = fon::yaml::serialize(data);

    REQUIRE(node.IsMap());
    REQUIRE(node.size() == 1);

    REQUIRE(node[1].IsScalar());
    REQUIRE(node[1].as<int32_t>() == 42);
}
TEST_CASE("test yaml deserialization of std::variant (index 1)", "[yaml][std][variant][deserialize]") {
    YAML::Node node {YAML::NodeType::Map};
    node[1] = 42;
    auto data = fon::yaml::deserialize<std::variant<std::string, int32_t, bool>>(node);
    REQUIRE(data.index() == 1);
    REQUIRE(std::get<1>(data) == 42);
}

TEST_CASE("test yaml serialization of std::variant (index 2)", "[yaml][std][variant][serialize]") {
    auto data = std::variant<std::string, int32_t, bool>{true};
    auto node = fon::yaml::serialize(data);

    REQUIRE(node.IsMap());
    REQUIRE(node.size() == 1);

    REQUIRE(node[2].IsScalar());
    REQUIRE(node[2].as<bool>() == true);
}
TEST_CASE("test yaml deserialization of std::variant (index 2)", "[yaml][std][variant][deserialize]") {
    YAML::Node node {YAML::NodeType::Map};
    node[2] = true;
    auto data = fon::yaml::deserialize<std::variant<std::string, int32_t, bool>>(node);
    REQUIRE(data.index() == 2);
    REQUIRE(std::get<2>(data) == true);
}

TEST_CASE("test yaml serialization of std::filesystem::path", "[yaml][std][filesystem][path][serialize]") {
    auto data = std::filesystem::path{"./myfile.txt"};
    auto node = fon::yaml::serialize(data);

    REQUIRE(node.IsScalar());
    REQUIRE(node.as<std::string>() == "./myfile.txt");
}

TEST_CASE("test yaml deserialization of std::filesystem::path", "[yaml][std][filesystem][path][deserialize]") {
    YAML::Node node;
    node = "./some_file";
    auto data = fon::yaml::deserialize<std::filesystem::path>(node);

    REQUIRE(data == "./some_file");
}

TEST_CASE("test yaml serialization of std::set<std::filesystem::path>", "[yaml][std][filesystem][set][path][serialize]") {
    auto data = std::set<std::filesystem::path>{{"./myfile.txt"}};

    auto node = fon::yaml::serialize(data);

    REQUIRE(node.IsSequence());
    REQUIRE(node[0].IsScalar());
    REQUIRE(node[0].as<std::string>() == "./myfile.txt");
}

TEST_CASE("test yaml deserialization of std::set<std::filesystem::path>", "[yaml][std][filesystem][set][path][deserialize]") {
    YAML::Node node;
    node[0] = "./some_file";
    auto data = fon::yaml::deserialize<std::set<std::filesystem::path>>(node);

    REQUIRE(data.size() == 1);
    REQUIRE(*data.begin() == "./some_file");
}

TEST_CASE("test yaml serialization of std::chrono::time_point<ms>", "[yaml][std][chrono][time_point][serialize]") {
    auto data = std::chrono::time_point<std::chrono::milliseconds>(std::chrono::milliseconds{42});
    auto node = fon::yaml::serialize(data);

    REQUIRE(node.IsScalar());
    REQUIRE(node.as<int>() == 42);
}

TEST_CASE("test yaml deserialization of std::chrono::time_point<ms>", "[yaml][std][chrono][time_point][deserialize]") {
    YAML::Node node;
    node = 42;
    auto data = fon::yaml::deserialize<std::chrono::time_point<std::chrono::milliseconds>>(node);
    REQUIRE(data == std::chrono::time_point<std::chrono::milliseconds>(std::chrono::milliseconds{42}));
}

TEST_CASE("test yaml serialization of std::chrono::time_point<ns>", "[yaml][std][chrono][time_point][serialize]") {
    auto data = std::chrono::time_point<std::chrono::nanoseconds>(std::chrono::nanoseconds{42});
    auto node = fon::yaml::serialize(data);

    REQUIRE(node.IsScalar());
    REQUIRE(node.as<int>() == 42);
}

TEST_CASE("test yaml deserialization of std::chrono::time_point<ns>", "[yaml][std][chrono][time_point][deserialize]") {
    YAML::Node node;
    node = 42;
    auto data = fon::yaml::deserialize<std::chrono::time_point<std::chrono::nanoseconds>>(node);
    REQUIRE(data == std::chrono::time_point<std::chrono::nanoseconds>(std::chrono::nanoseconds{42}));
}

TEST_CASE("test yaml serialization of std::filesystem::file_time_type", "[yaml][std][chrono][time_point][serialize]") {
    auto data = std::filesystem::file_time_type(std::chrono::nanoseconds{42});
    auto node = fon::yaml::serialize(data);

    REQUIRE(node.IsScalar());
    REQUIRE(node.as<int>() == 42);
}

TEST_CASE("test yaml deserialization of std::filesystem::file_time_type", "[yaml][std][chrono][time_point][deserialize]") {
    YAML::Node node;
    node = 42;
    auto data = fon::yaml::deserialize<std::filesystem::file_time_type>(node);
    REQUIRE(data == std::filesystem::file_time_type(std::chrono::nanoseconds{42}));
}

TEST_CASE("test yaml serialization of std::chrono::duration", "[yaml][std][chrono][duration][serialize]") {
    using namespace std::chrono_literals;
    auto data = 42ms - 20ms;
    auto node = fon::yaml::serialize(data);

    REQUIRE(node.IsScalar());
    REQUIRE(node.as<int>() == 22);
}

TEST_CASE("test yaml deserialization of std::chrono::duration", "[yaml][std][chrono][duration][deserialize]") {
    using namespace std::chrono_literals;

    YAML::Node node;
    node = 22;
    using duration = decltype(42ms - 20ms);
    auto data = fon::yaml::deserialize<duration>(node);
    REQUIRE(data == (42ms - 20ms));
}
