#include <iostream>
#include <serializer/serializer.h>

using SY = serializer::yaml::Serializer;
using DY = serializer::yaml::Deserializer;

struct A {
	int32_t x;
	int32_t y;

	template<typename Node>
	void serialize(Node& node) {
		node["y"] % y;
		node["x"] % x;
	}
};

int main(int, char**) {
	A a;
	auto out = serializer::yaml::writeAsString(a);
	std::cout << out << std::endl;
	return 0;
}

