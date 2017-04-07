#include <serializer/serializer.h>
#include <gtest/gtest.h>

#include <iostream>

template<typename T1, typename T2>
class TestF {
public:
	template<typename In, typename Out>
	static void run(In& _in, Out& _out) {
		T1 bs1;
		bs1.getRootNode() % _in;
		bs1.close();

		//std::cout<<"json:"<<std::endl;
		//std::cout<<(char*)bs1.getData().data()<<std::endl;
		//std::cout<<":json"<<std::endl;
		//std::cout << "yaml:\n" << bs1.getDataAsStr() << std::endl;


		T2 bs2 (bs1.getData());

		bs2.getRootNode() % _out;
		bs2.close();
	}
	template<typename In>
	static void runEQ(In const& _in) {
		In _out;
		run(_in, _out);
		EXPECT_EQ(_in, _out);
		if (_in != _out) throw "";
	}

};

using SB = serializer::binary::Serializer;
using DB = serializer::binary::Deserializer;
using SJ = serializer::json::Serializer;
using DJ = serializer::json::Deserializer;
using SY = serializer::yaml::Serializer;
using DY = serializer::yaml::Deserializer;


struct A {
	int32_t x;
	template<typename Node>
	void serialize(Node& node) {
		node["x"] % x;
	}
};
struct B {
	A  a;
	A* ptr {&a};
	template<typename Node>
	void serialize(Node& node) {
		node["a"]   % a;
		node["ptr"] % ptr;
	}
};
struct AD {
	A a;
	A* aPtr;
	int32_t* xPtr;

	template<typename Node>
	void serialize(Node& node) {
		node["a"] % a;
		node["aPtr"] % aPtr;
		node["xPtr"] % xPtr;
	}
};

struct LoopB_0;
struct LoopA_0 {
	int32_t x;
	LoopB_0* ptr;
	template<typename Node>
	void serialize(Node& node) {
		node["x"]   % x;
		node["ptr"] % ptr;
	}

	bool operator==(LoopA_0 const& _other) const {
		return ptr == _other.ptr;
	}
};

struct LoopB_0 {
	int32_t y;
	LoopA_0* ptr;
	template<typename Node>
	void serialize(Node& node) {
		node["y"]   % y;
		node["ptr"] % ptr;
	}
	bool operator==(LoopB_0 const& _other) const {
		return ptr == _other.ptr;
	}
};

struct Loop_0 {
	LoopA_0 a;
	LoopB_0 b;
	template<typename Node>
	void serialize(Node& node) {
		node["a"] % a;
		node["b"] % b;
	}

	bool operator==(Loop_0 const& _other) const {
		return a == _other.a and b == _other.b;
	}

};



struct LoopB_1;
struct LoopA_1 {
	int32_t x;
	LoopB_1* ptr;
	template<typename Node>
	void serialize(Node& node) {
		node["ptr"] % ptr;
	}

	bool operator==(LoopA_1 const& _other) const {
		return ptr == _other.ptr;
	}
};

struct LoopB_1 {
	int32_t y;
	LoopA_1* ptr;
	template<typename Node>
	void serialize(Node& node) {
		node["ptr"] % ptr;
	}
	bool operator==(LoopB_1 const& _other) const {
		return ptr == _other.ptr;
	}
};

struct Loop_1 {
	LoopA_1 a;
	LoopB_1 b;
	template<typename Node>
	void serialize(Node& node) {
		node["a"] % a;
		node["b"] % b;
	}

	bool operator==(Loop_1 const& _other) const {
		return a == _other.a and b == _other.b;
	}

};


struct LoopB_2;
struct LoopA_2 {
	LoopB_2* ptr;
	template<typename Node>
	void serialize(Node& node) {
		node["ptr"] % ptr;
	}

	bool operator==(LoopA_2 const& _other) const {
		return ptr == _other.ptr;
	}
};

struct LoopB_2 {
	LoopA_2* ptr;
	template<typename Node>
	void serialize(Node& node) {
		node["ptr"] % ptr;
	}
	bool operator==(LoopB_2 const& _other) const {
		return ptr == _other.ptr;
	}
};

struct Loop_2 {
	LoopA_2 a;
	LoopB_2 b;
	template<typename Node>
	void serialize(Node& node) {
		node["a"] % a;
		node["b"] % b;
	}

	bool operator==(Loop_2 const& _other) const {
		return a == _other.a and b == _other.b;
	}

};







template<typename S, typename D>
void fullTest() {
	{
		A in, out;
		in.x = 0x12345678;

		TestF<S, D>::run(in, out);
		EXPECT_EQ(in.x, out.x);

	}

	{
		B in, out;
		in.a.x = 0x12345678;
		out.ptr = nullptr;

		TestF<S, D>::run(in, out);
		EXPECT_EQ(&in.a, in.ptr);
		EXPECT_EQ(&out.a, out.ptr);

	}
	{
		AD in, out;
		in.aPtr = &in.a;
		in.xPtr = &in.a.x;

		TestF<S, D>::run(in, out);
		EXPECT_EQ(in.aPtr, &in.a);
		EXPECT_EQ(in.xPtr, &in.a.x);

		EXPECT_EQ(out.aPtr, &out.a);
		EXPECT_EQ(out.xPtr, &out.a.x);

	}

	{
		Loop_0 in;
		Loop_0 out;
		in.a.ptr = &in.b;
		in.b.ptr = &in.a;

		EXPECT_EQ(in.a.ptr, &in.b);
		EXPECT_EQ(in.b.ptr, &in.a);

		S bs1;
		bs1.getRootNode() % in;
		bs1.close();

//		std::cout<<(char*)bs1.getData().data()<<std::endl;

		D bs2 (bs1.getData());

		bs2.getRootNode() % out;
		bs2.close();

		EXPECT_EQ(in.a.ptr, &in.b);
		EXPECT_EQ(in.b.ptr, &in.a);

		EXPECT_EQ(out.a.ptr, &out.b);
		EXPECT_EQ(out.b.ptr, &out.a);
	}
	{
		Loop_1 in;
		Loop_1 out;
		in.a.ptr = &in.b;
		in.b.ptr = &in.a;

		EXPECT_EQ(in.a.ptr, &in.b);
		EXPECT_EQ(in.b.ptr, &in.a);

		S bs1;
		bs1.getRootNode() % in;
		bs1.close();

//		std::cout<<(char*)bs1.getData().data()<<std::endl;

		D bs2 (bs1.getData());

		bs2.getRootNode() % out;
		bs2.close();

		EXPECT_EQ(in.a.ptr, &in.b);
		EXPECT_EQ(in.b.ptr, &in.a);

		EXPECT_EQ(out.a.ptr, &out.b);
		EXPECT_EQ(out.b.ptr, &out.a);
	}

	{
		Loop_2 in;
		Loop_2 out;
		in.a.ptr = &in.b;
		in.b.ptr = &in.a;

		EXPECT_EQ(in.a.ptr, &in.b);
		EXPECT_EQ(in.b.ptr, &in.a);

		S bs1;
		bs1.getRootNode() % in;
		bs1.close();

//		std::cout<<(char*)bs1.getData().data()<<std::endl;

		D bs2 (bs1.getData());

		bs2.getRootNode() % out;
		bs2.close();

		EXPECT_EQ(in.a.ptr, &in.b);
		EXPECT_EQ(in.b.ptr, &in.a);

		EXPECT_EQ(out.a.ptr, &out.b);
		EXPECT_EQ(out.b.ptr, &out.a);
	}

}


TEST(TestPointers, TestPointers) {
	fullTest<SB, DB>();
	fullTest<SJ, DJ>();
	fullTest<SY, DY>();
}


struct C {
	std::unique_ptr<A> a { new A() };
	template<typename Node>
	void serialize(Node& node) {
		node["a"] % a;
	}
};

struct C_SPtr {
	std::shared_ptr<A> a { std::make_shared<A>() };
	template<typename Node>
	void serialize(Node& node) {
		node["a"] % a;
	}
};



template<typename S, typename D>
void fullTestSmartPointers() {
	{
		C in, out;
		in.a->x  = 5;
		out.a->x = 0;
		TestF<S, D>::run(in, out);
		EXPECT_EQ(in.a->x, 5);
		EXPECT_NE(out.a, nullptr);
		EXPECT_EQ(out.a->x, in.a->x);
	}
	{
		C_SPtr in, out;
		in.a->x  = 5;
		out.a->x = 0;
		EXPECT_NE(out.a, nullptr);

		TestF<S, D>::run(in, out);
		EXPECT_EQ(in.a->x, 5);
		EXPECT_NE(out.a, nullptr);

		EXPECT_EQ(out.a->x, in.a->x);
	}

}

TEST(TestPointers, TestUniquePtrs) {
	fullTestSmartPointers<SB, DB>();
	fullTestSmartPointers<SJ, DJ>();
	fullTestSmartPointers<SY, DY>();
}

