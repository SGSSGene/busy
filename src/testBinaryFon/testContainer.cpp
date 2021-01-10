#include <catch/catch.hpp>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>
#include <flattensObjectsNeatly/filesystem.h>
#include <flattensObjectsNeatly/chrono.h>

namespace {

template <typename ...Ts>
auto createVector(Ts... ts) {
	return std::vector<std::byte>{std::byte{uint8_t(ts)}...};
}

TEST_CASE("test binary serialization of std::string", "[serialize][binary][std][string]") {
	auto data = std::string{"hallo welt"};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     19, 0, 0, 0, 0, 0, 0, 0,
	      1, 'h', 'a', 'l', 'l', 'o', ' ', 'w', 'e', 'l', 't'
	);

	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}
TEST_CASE("test binary deserialization of std::string", "[deserialize][binary][std][string]") {
	auto buffer = fon::binary::serialize(std::string{"hallo welt"});
	auto data = fon::binary::deserialize<std::string>(buffer);
	CHECK(data.size() == std::string{"hallo welt"}.size());
	CHECK(data == "hallo welt");
}

TEST_CASE("test binary serialization of empty std::vector", "[serialize][binary][std][vector][empty]") {
	auto data = std::vector<int32_t>{};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     17, 0, 0, 0, 0, 0, 0, 0,
	      2, 0, 0, 0, 0, 0, 0, 0, 0 // sequence(2) with 0 entry
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}

TEST_CASE("test binary deserialization of empty std::vector", "[deserialize][binary][std][vector][empty]") {
	auto buffer = fon::binary::serialize(std::vector<int32_t>{});
	auto data = fon::binary::deserialize<std::vector<int32_t>>(buffer);
	REQUIRE(data == (std::vector<int32_t>{}));
}

TEST_CASE("test binary serialization of std::vector", "[binary][std][vector]") {
	auto data = std::vector<int32_t>{10, 20, 30};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     56, 0, 0, 0, 0, 0, 0, 0,
	      2, 3, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 3 entries
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      10, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      20, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      30, 0, 0, 0
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}
TEST_CASE("test binary deserialization of std::vector", "[binary][std][vector]") {
	auto buffer = fon::binary::serialize(std::vector<int32_t>{10,20, 30});
	auto data = fon::binary::deserialize<std::vector<int32_t>>(buffer);
	REQUIRE(data == (std::vector<int32_t>{10, 20, 30}));
}
TEST_CASE("test binary serialization of std::array", "[binary][std][array]") {
	auto data = std::array<int32_t, 3>{10, 20, 30};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     56, 0, 0, 0, 0, 0, 0, 0,
	      2, 3, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 3 entries
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      10, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      20, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      30, 0, 0, 0
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}
TEST_CASE("test binary deserialization of std::array", "[binary][std][array]") {
	auto buffer = fon::binary::serialize(std::array<int32_t, 3>{10,20, 30});
	auto data = fon::binary::deserialize<std::array<int32_t, 3>>(buffer);
	REQUIRE(data == (std::array<int32_t, 3>{10, 20, 30}));
}

TEST_CASE("test binary serialization of std::list", "[binary][std][list]") {
	auto data = std::list<int32_t>{10, 20, 30};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     56, 0, 0, 0, 0, 0, 0, 0,
	      2, 3, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 3 entries
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      10, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      20, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      30, 0, 0, 0
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}
TEST_CASE("test binary deserialization of std::list", "[binary][std][list]") {
	auto buffer = fon::binary::serialize(std::list<int32_t>{10,20, 30});
	auto data = fon::binary::deserialize<std::list<int32_t>>(buffer);
	REQUIRE(data == (std::list<int32_t>{10, 20, 30}));
}

TEST_CASE("test binary serialization of std::forward_list", "[binary][std][forward_list]") {
	auto data = std::forward_list<int32_t>{10, 20, 30};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     56, 0, 0, 0, 0, 0, 0, 0,
	      2, 3, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 3 entries
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      10, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      20, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      30, 0, 0, 0
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}
TEST_CASE("test binary deserialization of std::forward_list", "[binary][std][forward_list]") {
	auto buffer = fon::binary::serialize(std::forward_list<int32_t>{10,20, 30});
	auto data = fon::binary::deserialize<std::forward_list<int32_t>>(buffer);
	REQUIRE(data == (std::forward_list<int32_t>{10, 20, 30}));
}

TEST_CASE("test binary serialization of std::deque", "[binary][std][deque]") {
	auto data = std::deque<int32_t>{10, 20, 30};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     56, 0, 0, 0, 0, 0, 0, 0,
	      2, 3, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 3 entries
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      10, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      20, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      30, 0, 0, 0
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}
TEST_CASE("test binary deserialization of std::deque", "[binary][std][deque]") {
	auto buffer = fon::binary::serialize(std::deque<int32_t>{10,20, 30});
	auto data = fon::binary::deserialize<std::deque<int32_t>>(buffer);
	REQUIRE(data == (std::deque<int32_t>{10, 20, 30}));
}

TEST_CASE("test binary serialization of std::set", "[binary][std][set]") {
	auto data = std::set<int32_t>{10, 20, 30};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     56, 0, 0, 0, 0, 0, 0, 0,
	      2, 3, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 3 entries
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      10, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      20, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      30, 0, 0, 0
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}
TEST_CASE("test binary deserialization of std::set", "[binary][std][set]") {
	auto buffer = fon::binary::serialize(std::set<int32_t>{10,20, 30});
	auto data = fon::binary::deserialize<std::set<int32_t>>(buffer);
	REQUIRE(data == (std::set<int32_t>{10, 20, 30}));
}

TEST_CASE("test binary serialization of std::unordered_set", "[binary][std][unordered_set]") {
	auto data = std::unordered_set<int32_t>{10, 20, 30};
	auto buffer = fon::binary::serialize(data);
	auto expect1 = createVector(
	     56, 0, 0, 0, 0, 0, 0, 0,
	      2, 3, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 3 entries
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      10, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      20, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      30, 0, 0, 0
	);
	auto expect2 = createVector(
	     56, 0, 0, 0, 0, 0, 0, 0,
	      2, 3, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 3 entries
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      20, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      10, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      30, 0, 0, 0
	);

	auto expect3 = createVector(
	     56, 0, 0, 0, 0, 0, 0, 0,
	      2, 3, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 3 entries
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      20, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      30, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      10, 0, 0, 0
	);
	auto expect4 = createVector(
	     56, 0, 0, 0, 0, 0, 0, 0,
	      2, 3, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 3 entries
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      10, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      30, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      20, 0, 0, 0
	);
	auto expect5 = createVector(
	     56, 0, 0, 0, 0, 0, 0, 0,
	      2, 3, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 3 entries
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      30, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      10, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      20, 0, 0, 0
	);
	auto expect6 = createVector(
	     56, 0, 0, 0, 0, 0, 0, 0,
	      2, 3, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 3 entries
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      30, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      20, 0, 0, 0,
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      10, 0, 0, 0
	);





	CHECK(buffer.size() == expect1.size());
	CHECK((buffer == expect1
	      or buffer == expect2
	      or buffer == expect3
	      or buffer == expect4
	      or buffer == expect4
	      or buffer == expect6));
}
TEST_CASE("test binary deserialization of std::unordered_set", "[binary][std][unordered_set]") {
	auto buffer = fon::binary::serialize(std::unordered_set<int32_t>{10,20, 30});
	auto data = fon::binary::deserialize<std::unordered_set<int32_t>>(buffer);
	REQUIRE(data == (std::unordered_set<int32_t>{10, 20, 30}));
}

TEST_CASE("test binary serialization of empty std::map", "[serialize][binary][std][map][empty]") {
	auto data = std::map<std::string, int32_t>{};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     17, 0, 0, 0, 0, 0, 0, 0,
	      3, 0, 0, 0, 0, 0, 0, 0, 0 // map(3) with 0 entry
	);

	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}
TEST_CASE("test binary deserialization of empty std::map", "[deserialize][binary][std][map][empty]") {
	auto input = std::map<std::string, int32_t>{};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<std::map<std::string, int32_t>>(buffer);

	CHECK(data.size() == input.size());
	CHECK(data == input);
}


TEST_CASE("test binary serialization of std::map", "[binary][std][map]") {
	auto data = std::map<std::string, int32_t>{{"k1", 10}, {"k2", 20}, {"k3", 30}};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     89, 0, 0, 0, 0, 0, 0, 0,
	      3, 3, 0, 0, 0, 0, 0, 0, 0, // map(3) with 3 entries
	     11, 0, 0, 0, 0, 0, 0, 0, // key 1
	      1, 'k', '1',
	     13, 0, 0, 0, 0, 0, 0, 0, // value
	      1, 10, 0, 0, 0,
	     11, 0, 0, 0, 0, 0, 0, 0, // key 2
	      1, 'k', '2',
	     13, 0, 0, 0, 0, 0, 0, 0, // value
	      1, 20, 0, 0, 0,
	     11, 0, 0, 0, 0, 0, 0, 0, // key 3
	      1, 'k', '3',
	     13, 0, 0, 0, 0, 0, 0, 0, // value
	      1, 30, 0, 0, 0
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer.size() == int(buffer.at(0)));
	CHECK(buffer == expect);
}
TEST_CASE("test binary deserialization of std::map", "[binary][std][map]") {
	auto input = std::map<std::string, int32_t>{{"k1", 10}, {"k2", 20}, {"k3", 30}};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<std::map<std::string, int32_t>>(buffer);

	CHECK(data.size() == input.size());
	CHECK(data == input);
}

TEST_CASE("test binary serialization of std::unordered_map", "[binary][std][unordered_map]") {
	auto data = std::unordered_map<std::string, int32_t>{{"k1", 10}, {"k2", 20}, {"k3", 30}};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     89, 0, 0, 0, 0, 0, 0, 0,
	      3, 3, 0, 0, 0, 0, 0, 0, 0, // unordered_map(3) with 3 entries
	     11, 0, 0, 0, 0, 0, 0, 0, // key 1
	      1, 'k', '1',
	     13, 0, 0, 0, 0, 0, 0, 0, // value
	      1, 10, 0, 0, 0,
	     11, 0, 0, 0, 0, 0, 0, 0, // key 2
	      1, 'k', '2',
	     13, 0, 0, 0, 0, 0, 0, 0, // value
	      1, 20, 0, 0, 0,
	     11, 0, 0, 0, 0, 0, 0, 0, // key 3
	      1, 'k', '3',
	     13, 0, 0, 0, 0, 0, 0, 0, // value
	      1, 30, 0, 0, 0
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer.size() == int(buffer.at(0)));
	//CHECK(buffer == expect);//!TODO how to handle permutations?
}
TEST_CASE("test binary deserialization of std::unordered_map", "[binary][std][unordered_map]") {
	auto input = std::unordered_map<std::string, int32_t>{{"k1", 10}, {"k2", 20}, {"k3", 30}};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<std::unordered_map<std::string, int32_t>>(buffer);

	CHECK(data.size() == input.size());
	CHECK(data == input);
}

TEST_CASE("test binary serialization of std::pair", "[binary][std][pair]") {
	auto data = std::pair<std::string, int32_t>{"hallo welt", 42};
	auto buffer = fon::binary::serialize(data);

	auto expect = createVector(
	     49, 0, 0, 0, 0, 0, 0, 0,
	      2, 2, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 2 entries
	      19, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      'h', 'a', 'l', 'l', 'o', ' ', 'w', 'e', 'l', 't',
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      42, 0, 0, 0
	);
	CHECK(buffer.size() == int(buffer.at(0)));
	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}

TEST_CASE("test binary deserialization of std::pair", "[binary][std][pair]") {
	auto input = std::pair<std::string, int32_t>{"hallo welt", 42};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<std::pair<std::string, int32_t>>(buffer);
	REQUIRE(data == input);
}


TEST_CASE("test binary serialization of std::tuple<>", "[binary][std][tuple]") {
	auto data = std::tuple<>{};
	auto buffer = fon::binary::serialize(data);

	auto expect = createVector(
	     17, 0, 0, 0, 0, 0, 0, 0,
	      2, 0, 0, 0, 0, 0, 0, 0, 0 // map(2) with 0 entry
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}

TEST_CASE("test binary deserialization of std::tuple<>", "[binary][std][tuple]") {
	auto input = std::tuple<>{};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<std::tuple<>>(buffer);
	REQUIRE(data == input);
}

TEST_CASE("test binary serialization of std::tuple<X>", "[binary][std][tuple]") {
	auto data = std::tuple<std::string>{"hallo welt"};
	auto buffer = fon::binary::serialize(data);

	auto expect = createVector(
	     36, 0, 0, 0, 0, 0, 0, 0,
	      2, 1, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 1 entries
	      19, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      'h', 'a', 'l', 'l', 'o', ' ', 'w', 'e', 'l', 't'
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer.size() == int(buffer.at(0)));
	CHECK(buffer == expect);
}

TEST_CASE("test binary deserialization of std::tuple<X>", "[binary][std][tuple]") {
	auto input = std::tuple<std::string>{"hallo welt"};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<std::tuple<std::string>>(buffer);
	REQUIRE(data == input);
}

TEST_CASE("test binary serialization of std::tuple<X, Y>", "[binary][std][tuple]") {
	auto data = std::tuple<std::string, int32_t>{"hallo welt", 42};
	auto buffer = fon::binary::serialize(data);

	auto expect = createVector(
	     49, 0, 0, 0, 0, 0, 0, 0,
	      2, 2, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 2 entries
	      19, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      'h', 'a', 'l', 'l', 'o', ' ', 'w', 'e', 'l', 't',
	      13, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      42, 0, 0, 0
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer.size() == int(buffer.at(0)));
	CHECK(buffer == expect);
}

TEST_CASE("test binary deserialization of std::tuple<X, Y>", "[binary][std][tuple]") {
	auto input = std::tuple<std::string, int32_t>{"hallo welt", 42};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<std::tuple<std::string, int32_t>>(buffer);
	REQUIRE(data == input);
}

TEST_CASE("test binary serialization of std::optional", "[serialize][binary][std][optional]") {
	auto data = std::optional<std::string>{"hallo welt"};
	auto buffer = fon::binary::serialize(data);

	auto expect = createVector(
	     36, 0, 0, 0, 0, 0, 0, 0,
	      2, 1, 0, 0, 0, 0, 0, 0, 0, // sequence(2) with 1 entries
	      19, 0, 0, 0, 0, 0, 0, 0, 1, // size 4 of type value(1)
	      'h', 'a', 'l', 'l', 'o', ' ', 'w', 'e', 'l', 't'
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer.size() == int(buffer.at(0)));
	CHECK(buffer == expect);
}

TEST_CASE("test binary deserialization of std::optional", "[deserialize][binary][std][optional]") {
	auto input = std::optional<std::string>{"hallo welt"};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<std::optional<std::string>>(buffer);
	REQUIRE(data == input);
}

TEST_CASE("test binary serialization of empty std::optional", "[serialize][binary][std][optional]") {
	auto data = std::optional<std::string>{};
	auto buffer = fon::binary::serialize(data);

	auto expect = createVector(
	     17, 0, 0, 0, 0, 0, 0, 0,
	      2, 0, 0, 0, 0, 0, 0, 0, 0 // sequence(2) with 0 entry
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}
TEST_CASE("test binary deserialization of empty std::optional", "[deserialize][binary][std][optional]") {
	auto input = std::optional<std::string>{};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<std::optional<std::string>>(buffer);
	CHECK(not data);
}

TEST_CASE("test binary serialization of std::variant (index 0)", "[serialize][binary][std][variant]") {
	auto data = std::variant<std::string, int32_t, bool>{std::string{"hallo welt"}};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     53, 0, 0, 0, 0, 0, 0, 0,
	      3, 1, 0, 0, 0, 0, 0, 0, 0, // map(3) with 1 entry
	     17, 0, 0, 0, 0, 0, 0, 0, // key 1
	      1, 0, 0, 0, 0, 0, 0, 0, 0,
	     19, 0, 0, 0, 0, 0, 0, 0, // value
	      1, 'h', 'a', 'l', 'l', 'o', ' ', 'w', 'e', 'l', 't'
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer.size() == int(buffer.at(0)));
	CHECK(buffer == expect);
}

TEST_CASE("test binary deserialization of std::variant (index 0)", "[deserialize][binary][std][variant]") {
	auto input = std::variant<std::string, int32_t, bool>{std::string{"hallo welt"}};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<std::variant<std::string, int32_t, bool>>(buffer);

	CHECK(data.index() == 0);
	CHECK(data == input);
}

TEST_CASE("test binary serialization of std::variant (index 1)", "[binary][std][variant]") {
	auto data = std::variant<std::string, int32_t, bool>{42};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     47, 0, 0, 0, 0, 0, 0, 0,
	      3, 1, 0, 0, 0, 0, 0, 0, 0, // map(3) with 1 entry
	     17, 0, 0, 0, 0, 0, 0, 0, // key 1
	      1, 1, 0, 0, 0, 0, 0, 0, 0,
	     13, 0, 0, 0, 0, 0, 0, 0, // value
	      1, 42, 0, 0, 0
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer.size() == int(buffer.at(0)));
	CHECK(buffer == expect);
}

TEST_CASE("test binary deserialization of std::variant (index 1)", "[binary][std][variant]") {
	auto input = std::variant<std::string, int32_t, bool>{42};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<std::variant<std::string, int32_t, bool>>(buffer);

	CHECK(data.index() == 1);
	CHECK(data == input);
}

TEST_CASE("test binary serialization of std::variant (index 2)", "[binary][std][variant]") {
	auto data = std::variant<std::string, int32_t, bool>{true};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     44, 0, 0, 0, 0, 0, 0, 0,
	      3, 1, 0, 0, 0, 0, 0, 0, 0, // map(3) with 1 entry
	     17, 0, 0, 0, 0, 0, 0, 0, // key 1
	      1, 2, 0, 0, 0, 0, 0, 0, 0,
	     10, 0, 0, 0, 0, 0, 0, 0, // value
	      1, 1
	);
	CHECK(buffer.size() == expect.size());
	CHECK(buffer.size() == int(buffer.at(0)));
	CHECK(buffer == expect);
}

TEST_CASE("test binary deserialization of std::variant (index 2)", "[binary][std][variant]") {
	auto input = std::variant<std::string, int32_t, bool>{true};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<std::variant<std::string, int32_t, bool>>(buffer);

	CHECK(data.index() == 2);
	CHECK(data == input);
}

TEST_CASE("test binary serialization of std::filesystem::path", "[binary][std][filesystem][path]") {
	auto data = std::filesystem::path{"./myfile.txt"};
	auto buffer = fon::binary::serialize(data);
	auto expect = createVector(
	     21, 0, 0, 0, 0, 0, 0, 0,
	      1, '.', '/', 'm', 'y', 'f', 'i', 'l', 'e', '.', 't', 'x', 't'
	);

	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}

TEST_CASE("test binary deserialization of std::filesystem::path", "[binary][std][filesystem][path]") {
	auto buffer = fon::binary::serialize(std::filesystem::path{"./some_file"});
	auto data = fon::binary::deserialize<std::filesystem::path>(buffer);
	REQUIRE(data == "./some_file");
}

TEST_CASE("test binary deserialization of std::chrono::time_point<ms>", "[binary][std][chrono][time_point]") {
	auto input = std::chrono::time_point<std::chrono::milliseconds>(std::chrono::milliseconds{42});
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<decltype(input)>(buffer);
	REQUIRE(data == input);
}

TEST_CASE("test binary deserialization of std::chrono::time_point<ns>", "[binary][std][chrono][time_point]") {
	auto input = std::chrono::time_point<std::chrono::nanoseconds>(std::chrono::nanoseconds{42});
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<decltype(input)>(buffer);
	REQUIRE(data == input);
}

TEST_CASE("test binary deserialization of std::filesystem::file_time_type", "[binary][std][chrono][time_point]") {
	auto input = std::filesystem::file_time_type(std::chrono::nanoseconds{42});
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<decltype(input)>(buffer);
	REQUIRE(data == input);
}

	using AllIncludes = std::tuple<std::set<std::filesystem::path>, std::set<std::filesystem::path>>;
	using Hash = std::string;

	template <typename T>
	struct Pair {
		std::filesystem::file_time_type modTime{};
		T                               value{};

		template <typename Node>
		void serialize(Node& node) {
			node["modTime"] % modTime;
			node["value"]   % value;
		}
	};


	template <typename ...Args>
	using PairTuple = std::tuple<Pair<Args>...>;

TEST_CASE("test binary (de-)serialization of std::tuple with complex data types", "[binary][std][tuple][complex]") {
	using T =  std::tuple<std::string, std::string>;
	auto input = T{};

	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<decltype(input)>(buffer);
	REQUIRE(std::get<0>(data) == "");
	REQUIRE(std::get<1>(data) == "");
}



}
