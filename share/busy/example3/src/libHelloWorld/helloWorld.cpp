#include <libOne/libOne.h>
#include <libTwo/libTwo.h>
#include <iostream>

void helloWorld() {
	std::cout << "Hello World " << libOne() + libTwo() << "\n";
}
