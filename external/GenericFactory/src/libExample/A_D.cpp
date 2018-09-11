#include "A_D.h"

#include <iostream>

std::string D1_of_A::bar() {
	return "D1_of_A";
}
std::string D2_of_A::bar() {
	return "D2_of_A";
}


std::string D1_of_D1_of_A::bar() {
	return "D1_of_D1_of_A";
}

// Register class at factory
//namespace {
//	static genericFactory::Register<A>                         base("A");
	genericFactory::Register<D1_of_A, A>                item1("D1_of_A");
	genericFactory::Register<D2_of_A, A>                item2("D2_of_A");
	genericFactory::Register<D1_of_D1_of_A, D1_of_A, A> item3("D1_of_D1_of_A");
//}


