#include <unitTestLib/foo.h>
#include <unitTestStaticLib/bar.h>
#include <iostream>

int main() {
	std::cout << foo() << std::endl;
	std::cout << bar() << std::endl;
	return 0;
}
