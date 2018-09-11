/** \example src/testGenericFactory/test.cpp
 */

#include <iostream>
#include <gtest/gtest.h>
#include <genericFactory/genericFactory.h>
#include <libExample/A.h>

class B {
public:
	virtual ~B() {}
	virtual std::string bar() = 0;
};

class D1_of_B : public B {
public:
	std::string bar() override {
		return "D1_of_B";
	}
};

class D2_of_B final : public B {
public:
	std::string bar() override {
		return "D2_of_B";
	}
};

class D1_of_D1_of_B : public D1_of_B {
public:
	std::string bar() override {
		return "D1_of_D1_of_B";
	}
};

class C {
public:
	virtual ~C() {}
};


// Register class at factory
namespace {
	genericFactory::Register<B>                         base("B");
	genericFactory::Register<D1_of_B, B>                item1("D1_of_B");
	genericFactory::Register<D2_of_B, B>                item2("D2_of_B");
	genericFactory::Register<D1_of_D1_of_B, D1_of_B, B> item3("D1_of_D1_of_B");
}

TEST(Test, Test0) {
	std::shared_ptr<B> ptr1 = genericFactory::make_shared<B>("D1_of_B");
	std::unique_ptr<B> ptr2 = genericFactory::make_unique<B>("D1_of_B");

	EXPECT_EQ(ptr1->bar(), "D1_of_B");
	EXPECT_EQ(ptr2->bar(), "D1_of_B");
}

TEST(Test, Test1) {
	auto ptr = genericFactory::make_shared<B>("D1_of_B");
	EXPECT_EQ(ptr->bar(), "D1_of_B");
}

TEST(Test, Test2) {
	auto ptr = genericFactory::make_shared<B>("D1_of_D1_of_B");
	EXPECT_EQ(ptr->bar(), "D1_of_D1_of_B");
}

TEST(Test, Test3) {
	auto ptr = genericFactory::make_shared<B>("D2_of_B");
	EXPECT_EQ(ptr->bar(), "D2_of_B");
}
TEST(Test, Test4) {
	EXPECT_ANY_THROW(genericFactory::make_shared<D2_of_B>("Deriv1_Deriv1_of_Base1"));
}
TEST(Test, Test5) {
	auto ptr = genericFactory::make_unique<B>("D1_of_B");
	EXPECT_EQ(ptr->bar(), "D1_of_B");
}

TEST(Test, Test6) {
	auto ptr = genericFactory::make_unique<B>("D1_of_D1_of_B");
	EXPECT_EQ(ptr->bar(), "D1_of_D1_of_B");
}

TEST(Test, Test7) {
	auto ptr = genericFactory::make_unique<B>("D2_of_B");
	EXPECT_EQ(ptr->bar(), "D2_of_B");
}
TEST(Test, Test8) {
	EXPECT_ANY_THROW(genericFactory::make_unique<D2_of_B>("Deriv1_Deriv1_of_Base1"));
}

TEST(Test, Test9) {
	auto names = genericFactory::getClassList<B>();
	std::set<std::string> expect {"D1_of_B", "D2_of_B", "D1_of_D1_of_B"};
	EXPECT_EQ(expect, names);
}

TEST(Test, Test10) {
	auto names = genericFactory::getClassList<A>();
	std::set<std::string> expect {"D1_of_A", "D2_of_A", "D1_of_D1_of_A"};
	EXPECT_EQ(expect, names);
}

TEST(Test, Test11) {
	EXPECT_EQ(0, genericFactory::getClassList<C>().size());
	{
		genericFactory::Register<C> base("C");
		EXPECT_EQ(1, genericFactory::getClassList<C>().size());
	}
	EXPECT_EQ(0, genericFactory::getClassList<C>().size());
}









