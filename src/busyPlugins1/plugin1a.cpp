#include <busyPluginsBase/Base.h>
#include <iostream>
namespace {
	class D : public Base {
	public:
		D() {
			std::cout << "D is running\n";
		}
		~D() {
		}
	};

	genericFactory::Register<D, Base>                item1("D");
}

