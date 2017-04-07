#include "A.h"
class D1_of_A : public A {
public:
	std::string bar() override;
};

class D2_of_A final : public A {
public:
	std::string bar() override;
};

class D1_of_D1_of_A : public D1_of_A {
public:
	std::string bar() override;
};


