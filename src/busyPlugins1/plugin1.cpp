#include <busyPluginsBase/Base.h>
#include <iostream>
namespace {
	class A : public Base {
	public:
		A() {
			std::cout << "Hallo Welt, ich bin ein Plugin installer\n";
		}
		~A() {
			std::cout << "Jetzt muss ich auch schon wieder gehen\n";
		}
	};
	A a;
	genericFactory::Register<A, Base>                item1("A");
}
	struct C : public Base {
		C() {
			std::cout << "C in A\n";
		}
	};
	C c;
	genericFactory::Register<C, Base>                item1("C");


