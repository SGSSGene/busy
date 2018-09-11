#pragma once
#include <genericFactory/genericFactory.h>

class A {
public:
	virtual ~A() {}
	virtual std::string bar() = 0;
};


