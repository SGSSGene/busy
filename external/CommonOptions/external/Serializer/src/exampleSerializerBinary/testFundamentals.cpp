#include <iostream>
#include <serializer/serializer.h>

using SY = serializer::binary::Serializer;
using DY = serializer::binary::Deserializer;

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
		node["z"] % z or 0;
		node["x"] % x;
	}
};

int main(int, char**) {
	A a;

	a.x = 1.;
	a.y = 2.;

	auto out = serializer::binary::write(a);
	//std::cout << out << std::endl;
	std::cout << "A has: " << a.x << " " << a.y << std::endl;

	std::map<std::string, std::map<std::string, std::vector<uint8_t>>> mUnused;
	B b;
	serializer::binary::read(out, b, false, &mUnused);
	std::cout << "x, z: " << b.x << " " << b.z << "\n";

	for (auto const& s : mUnused) {
		for (auto const& t : s.second) {
			if (s.first != "") {
				std::cout << "unused: " << s.first << "." << t.first << std::endl;
			} else {
				std::cout << "unused: " << t.first << std::endl;
			}
		}
	}

	auto out2 = serializer::binary::write(b, false, &mUnused);
	{
		A a2;
		serializer::binary::read(out2, a2);
		std::cout << "now A has: " << a2.x << " " << a2.y << std::endl;


	}


	return 0;
}

