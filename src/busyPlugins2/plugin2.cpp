#include <busyPluginsBase/Base.h>

#include <iostream>


namespace {
	class B : public Base {
	public:
		B() {
			std::cout << "ich bin plugin2 und habe die addresse: " << this << "\n";
		}
		~B() {
		}
	};
	B b;
	genericFactory::Register<B, Base>                item1("B");
}
	struct C : public Base {
		C() {
			std::cout << "C in B\n";
		}
	};
	C c;
	genericFactory::Register<C, Base>                item1("C");

