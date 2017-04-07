#include <serializer/serializer.h>
#include <gtest/gtest.h>

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
	bool operator==(A const& _other) const {
		return x == _other.x;
	}
};

struct TwoValues_1 {
	int32_t a, b;

	template<typename Node>
	void serialize(Node& node) {
		node["a"] % a;
		node["b"] % b;
	}
	bool operator==(TwoValues_1 const& _other) const {
		return a == _other.a and b == _other.b;
	}
};
struct TwoValues_a {
	int32_t a;

	template<typename Node>
	void serialize(Node& node) {
		node["a"] % a;
	}
};
struct TwoValues_b {
	int32_t b;

	template<typename Node>
	void serialize(Node& node) {
		node["b"] % b;
	}
};
struct TwoValues_c {
	int32_t c;

	template<typename Node>
	void serialize(Node& node) {
		node["c"] % c;
	}
};

struct List {
	std::vector<int32_t> list;

	template<typename Node>
	void serialize(Node& node) {
		node["list"] % list or std::vector<int32_t>{};
	}

	bool operator==(List const& _other) const {
		return list == _other.list;
	}
	bool operator!=(List const& _other) const {
		return list != _other.list;
	}
};
struct List12 {
	std::vector<int32_t> list;

	template<typename Node>
	void serialize(Node& node) {
		node["list"] % list or std::vector<int32_t>{1, 2};
	}

	bool operator==(List12 const& _other) const {
		return list == _other.list;
	}
	bool operator!=(List12 const& _other) const {
		return list != _other.list;
	}
};

struct List12_nested {
	std::vector<List12> list;

	template<typename Node>
	void serialize(Node& node) {
		node["list_nested"] % list or std::vector<List12>{List12{{1, 2}}, List12{{1, 2}}};
	}

	bool operator==(List12_nested const& _other) const {
		return list == _other.list;
	}
	bool operator!=(List12_nested const& _other) const {
		return list != _other.list;
	}
};
struct List12_nested2 {
	std::vector<List12> list;

	template<typename Node>
	void serialize(Node& node) {
		node["list_nested2"] % list;
	}

	bool operator==(List12_nested2 const& _other) const {
		return list == _other.list;
	}
	bool operator!=(List12_nested2 const& _other) const {
		return list != _other.list;
	}
};


template<typename T1, typename T2>
class TestF {
public:
	template<typename In, typename Out>
	static void run(In _in, Out& _out) {
		T1 bs1;
		bs1.getRootNode() % _in;
		bs1.close();

//		std::cout << "yaml:\n" << bs2.getDataAsStr() << std::endl;

		T2 bs2 (bs1.getData());

		bs2.getRootNode() % _out;
		bs2.close();

/*		List12& in  = _in;
		List12& out = _out;
		std::cout<<"in list: "<< in.list.size() << std::endl;
		for (auto const& l : in.list) {
			std::cout << "  " << l << std::endl;
		}

		std::cout<<"out list: "<< out.list.size() << std::endl;
		for (auto const& l : out.list) {
			std::cout << "  " << l << std::endl;
		}*/


/*		List12_nested& in  = _in;
		List12_nested& out = _out;
		std::cout<<"in list: "<< in.list.size() << std::endl;
		for (auto const& l : in.list) {
			std::cout << " list 2: " << l.list.size()<< std::endl;
			for (auto const& l2 : l.list) {
				std::cout << "  " << l2 << std::endl;
			}
		}

		std::cout<<"out list: "<< out.list.size() << std::endl;
		for (auto const& l : out.list) {
			std::cout << " list 2: " << l.list.size()<< std::endl;
			for (auto const& l2 : l.list) {
				std::cout << "  " << l2 << std::endl;
			}
		}*/

	}
	template<typename In>
	static void runEQ(In const& _in) {
		In _out;
		run(_in, _out);
		EXPECT_EQ(_in, _out);
		if (_in != _out) throw "";
	}

};

template<typename S, typename D, typename T>
void testLimits(T const& _value) {
	TestF<S, D>::runEQ(_value);
	TestF<S, D>::runEQ(std::numeric_limits<T>::min());
	TestF<S, D>::runEQ(std::numeric_limits<T>::max());
}






template<typename S, typename D>
void fullTestFundamentals() {

	TestF<S, D>::runEQ(false);
	TestF<S, D>::runEQ(true);
	testLimits<S, D, uint8_t>(22);
	testLimits<S, D, int8_t>(22);
	testLimits<S, D, uint16_t>(22);
	testLimits<S, D, int16_t>(22);
	testLimits<S, D, uint32_t>(22);
	testLimits<S, D, int32_t>(22);
	testLimits<S, D, uint64_t>(22);
	testLimits<S, D, int64_t>(22);
	testLimits<S, D, float>(22.11);
	testLimits<S, D, double>(22.11);
	TestF<S, D>::runEQ(std::string("hello world"));
	TestF<S, D>::runEQ(std::vector<std::string> {{"hello world"}});
	TestF<S, D>::runEQ(std::map<int32_t, std::string> {{3, {"hello world"}}});
	TestF<S, D>::runEQ(std::map<int32_t, std::vector<std::string>> {{3, {{"hello world"}}}});
	using NodePath = std::vector<std::string>;
	TestF<S, D>::runEQ(std::map<int32_t, NodePath> {{3, {{"hello world"}}}});


	TestF<S, D>::runEQ(std::vector<int32_t> {0, 1, 2});
	TestF<S, D>::runEQ(std::vector<int32_t> {0, 1, 2});
	TestF<S, D>::runEQ(std::vector<double>  {0., 1., 2., 0.5});
	TestF<S, D>::runEQ(std::vector<double>  {});
	TestF<S, D>::runEQ(std::vector<float>   {});
	TestF<S, D>::runEQ(std::vector<uint8_t> {});
	TestF<S, D>::runEQ(std::list<uint8_t> {});
	TestF<S, D>::runEQ(std::list<uint8_t> {5});
	TestF<S, D>::runEQ(std::set<uint8_t> {});
	TestF<S, D>::runEQ(std::set<uint8_t> {5, 6, 7});

	TestF<S, D>::runEQ(std::pair<uint8_t, uint16_t> {5, 6});
	TestF<S, D>::runEQ(std::map <uint8_t, uint16_t> {{1, 2}, {3, 4}, {5, 6}});

	enum class MyEnum : uint16_t { A, B=5, C=3 };
	static_assert(std::is_enum<MyEnum>::value, "no enum");
	static_assert(not std::is_enum<int>::value, "no int");
	MyEnum myEnum = MyEnum::A;
	TestF<S, D>::runEQ(myEnum);

}
template<typename S, typename D>
void fullTestClasses() {
	{
		A in, out;
		in.x = 10;
		out.x = 20;
		TestF<S, D>::run(in, out);
		EXPECT_EQ(in.x, out.x);
	}
	TestF<S, D>::runEQ(std::vector<A> {{1}, {2}, {3}});


	{
		TwoValues_1 in, out;
		in.a = 10;  in.b = 20;
		out.a = 0; out.b = 0;
		TestF<S, D>::run(in, out);
		EXPECT_EQ(in.a, out.a);
		EXPECT_EQ(in.b, out.b);
	}
	TestF<S, D>::runEQ(std::vector<TwoValues_1> {{1, 2}, {2, 3}, {3, 4}});
	TestF<S, D>::runEQ(std::list<std::vector<TwoValues_1>> {{{1, 2}, {2, 3}, {3, 4}}});

	{
		TwoValues_1 in;
		TwoValues_a out;
		in.a = 10;  in.b = 20;
		out.a = 0;

		S bs1;
		bs1.getRootNode() % in;
		bs1.close();

		D bs2 (bs1.getData());

		bs2.getRootNode() % out;
		EXPECT_EQ(in.a, out.a);
	}
	{
		TwoValues_1 in;
		TwoValues_b out;
		in.a = 10;  in.b = 20;
		out.b = 0;

		S bs1;
		bs1.getRootNode() % in;
		bs1.close();

		D bs2 (bs1.getData());

		bs2.getRootNode() % out;
		EXPECT_EQ(in.b, out.b);
	}
	TestF<S, D>::runEQ(List{{}});
	TestF<S, D>::runEQ(List{{1}});
	TestF<S, D>::runEQ(List{{1, 2}});
	TestF<S, D>::runEQ(List{{1, 2, 3}});
	TestF<S, D>::runEQ(List12{{}});
	TestF<S, D>::runEQ(List12{{1}});
	TestF<S, D>::runEQ(List12{{1, 2}});
	TestF<S, D>::runEQ(List12{{1, 2, 3}});
	TestF<S, D>::runEQ(List12_nested{{}});
	TestF<S, D>::runEQ(List12_nested{{List12{{1, 2}}}});
	TestF<S, D>::runEQ(List12_nested{{List12{{1, 2}}, List12{{1, 2}}}});
	TestF<S, D>::runEQ(List12_nested2{{}});
	TestF<S, D>::runEQ(List12_nested2{{List12{{1, 2}}}});
	TestF<S, D>::runEQ(List12_nested2{{List12{{1, 2}}, List12{{1, 2}}}});
}


TEST(TestPrimitiveDataTypes, TestFundamentals) {
	fullTestFundamentals<SB, DB>();
	fullTestFundamentals<SJ, DJ>();
	fullTestFundamentals<SY, DY>();
}
TEST(TestPrimitiveDataTypes, TestClasses) {
	fullTestClasses<SB, DB>();
	fullTestClasses<SJ, DJ>();
	fullTestClasses<SY, DY>();
}

