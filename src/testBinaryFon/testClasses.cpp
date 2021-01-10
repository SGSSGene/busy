#include <catch/catch.hpp>
#include <flattensObjectsNeatly/flattensObjectsNeatly.h>

namespace {

template <typename ...Ts>
auto createVector(Ts... ts) {
	return std::vector<std::byte>{std::byte{uint8_t(ts)}...};
}


struct A {
	std::vector<int32_t> xs;

	template <typename Node>
	void serialize(Node& node) {
		node["xs"] % xs;
	}
};


TEST_CASE("test binary serialization of struct A", "[serialize][binary][struct]") {
	auto data = A{{10, 20, 30}};
	auto buffer = fon::binary::serialize(data);

	auto expect = createVector(
	     84, 0, 0, 0, 0, 0, 0, 0, // full size of map
	      3, 1, 0, 0, 0, 0, 0, 0, 0, // map(3) with 1 entry
	     11, 0, 0, 0, 0, 0, 0, 0, // key "xs"
	      1, 'x', 's',
	     56, 0, 0, 0, 0, 0, 0, 0, // value to "xs"
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

TEST_CASE("test binary deserialization of struct A", "[deserialize][binary][struct]") {
	auto input = A{{10, 20, 30}};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<A>(buffer);

	CHECK(data.xs == input.xs);
}

TEST_CASE("test binary serialization of empty struct A", "[serialize][binary][struct][empty]") {
	auto data = A{};
	auto buffer = fon::binary::serialize(data);

	auto expect = createVector(
	     45, 0, 0, 0, 0, 0, 0, 0, // full size of map
	      3, 1, 0, 0, 0, 0, 0, 0, 0, // map(3) with 1 entry
	     11, 0, 0, 0, 0, 0, 0, 0, // key "xs"
	      1, 'x', 's',
	     17, 0, 0, 0, 0, 0, 0, 0, // value to "xs"
	      2, 0, 0, 0, 0, 0, 0, 0, 0 // sequence(2) with 0 entries
	);

	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}

TEST_CASE("test binary deserialization of empty struct A", "[deserialize][binary][struct][empty]") {
	auto input = A{};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<A>(buffer);

	CHECK(data.xs == input.xs);
}


struct B {
	struct C {
		int32_t x;

		template <typename Node>
		void serialize(Node& node) {
			node["x"] % x;
		}
		bool operator==(C const& other) const noexcept {
			return x == other.x;
		}
	};
	std::map<std::string, C> infos;

	template <typename Node>
	void serialize(Node& node) {
		node["infos"] % infos;
	}
};

TEST_CASE("test binary serialization of maps with structs", "[binary][struct][std][map]") {
	auto data = B{};
	data.infos["k1"].x = 10;
	data.infos["k2"].x = 20;
	auto buffer = fon::binary::serialize(data);


	auto expect = createVector(
	     150, 0, 0, 0, 0, 0, 0, 0, // full size of map
	      3, 1, 0, 0, 0, 0, 0, 0, 0, // map(3) with 1 entry
	     14, 0, 0, 0, 0, 0, 0, 0, // key "infos"
	      1, 'i', 'n', 'f', 'o', 's',
	    119, 0, 0, 0, 0, 0, 0, 0, // value "infos"
	      3, 2, 0, 0, 0, 0, 0, 0, 0, // map(3) with 2 entries
	     11, 0, 0, 0, 0, 0, 0, 0, // key "k1"
	      1, 'k', '1',
	     40, 0, 0, 0, 0, 0, 0, 0, // value to "k1"
	      3, 1, 0, 0, 0, 0, 0, 0, 0, // map(3) with 1 entry
	      10, 0, 0, 0, 0, 0, 0, 0, // key "k1.x"
	      1, 'x',
	      13, 0, 0, 0, 0, 0, 0, 0, // value to "k1.x"
	      1, 10, 0, 0, 0,
	     11, 0, 0, 0, 0, 0, 0, 0, // key "k2"
	      1, 'k', '2',
	     40, 0, 0, 0, 0, 0, 0, 0, // value to "k2"
	      3, 1, 0, 0, 0, 0, 0, 0, 0, // map(3) with 1 entry
	      10, 0, 0, 0, 0, 0, 0, 0, // key "k2.x"
	      1, 'x',
	      13, 0, 0, 0, 0, 0, 0, 0, // "value "k2.x"
	      1, 20, 0, 0, 0
	);

	CHECK(buffer.size() == expect.size());
	CHECK(buffer == expect);
}
TEST_CASE("test binary deserialization of maps with structs", "[binary][struct][std][map]") {
	auto input = B{};
	input.infos["k1"].x = 10;
	input.infos["k2"].x = 20;
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<B>(buffer);

	CHECK(data.infos == input.infos);
}

struct D {
	std::vector<int32_t> xs;

	template <typename Node>
	void serialize(Node& node) {
		node % xs;
	}
};


TEST_CASE("test binary serialization of struct D (no name)", "[serialize][binary][struct][direct]") {
	auto data = D{{10, 20, 30}};
	auto buffer = fon::binary::serialize(data);

	auto expect = createVector(
	     56, 0, 0, 0, 0, 0, 0, 0, // value to "xs"
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

TEST_CASE("test binary deserialization of struct D (no name)", "[deserialize][binary][struct][direct]") {
	auto input = D{{10, 20, 30}};
	auto buffer = fon::binary::serialize(input);
	auto data = fon::binary::deserialize<D>(buffer);

	CHECK(data.xs == input.xs);
}

}
