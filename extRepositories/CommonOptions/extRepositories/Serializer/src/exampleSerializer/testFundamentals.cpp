#include <iostream>
#include <serializer/serializer.h>

using SY = serializer::yaml::Serializer;
using DY = serializer::yaml::Deserializer;

struct A {
	int32_t x{0};
	int32_t y{0};

	template<typename Node>
	void serialize(Node& node) {
		node["y"] % y;
		node["x"] % x;
	}
};

struct B {
	int32_t x{0};
	int32_t z{0};

	template<typename Node>
	void serialize(Node& node) {
		node["z"] % z;
		node["x"] % x;
	}
};

int main(int, char**) {
	A a;
	a.x = 1.;
	a.y = 2.;
	auto out = serializer::yaml::writeAsString(a);
	std::cout << out << std::endl;

	std::map<std::string, YAML::Node> mUnused;
	B b;
	serializer::yaml::readFromString(out, b, &mUnused);
	std::cout << "x, z: " << b.x << " " << b.z << "\n";

	for (auto const& s : mUnused) {
		for (auto const& t : s.second) {
			if (s.first != "") {
				std::cout << "unused: " << s.first << "." << t.first.as<std::string>() << std::endl;
			} else {
				std::cout << "unused: " << t.first.as<std::string>() << std::endl;
			}
		}
	}

	auto out2 = serializer::yaml::writeAsString(b, &mUnused);
	{
		A a2;
		serializer::yaml::readFromString(out2, a);
		std::cout << "now A has: " << a.x << " " << a.y << std::endl;


	}


	return 0;
}

