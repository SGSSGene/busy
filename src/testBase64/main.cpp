#include <base64/base64.h>
#include <cassert>

void test(std::string s) {
	auto encoded = base64::encode(s);
	auto decoded = base64::decode(encoded);
	auto len = base64::encoded_len(s);
	auto len2 = base64::decoded_len(encoded);
	assert(s == decoded);
	assert(encoded.size() == len);
	assert(base64::encoded_valid(encoded));
	assert(len2 == decoded.size());
}
int main() {
	test("Hallo Welt");
	test("mehr tests");
	test("");
	test("1");
	test("12");
	test("123");
	test("1234");
	test("12345");
	test("123456");
	test("1234567");
	test("12345678");
	test("123456789");
	test("1234567890");
	test("12345678901");
	test("123456789012");
	test("1234567890123");
	test("12345678901234");
	test("123456789012345");
	test("1234567890123456");
	test("12345678901234567");
	test("123456789012345678");
	test("1234567890123456789");
	test("12345678901234567890");
	test("123456789012345678901");
	test("1234567890123456789012");
	test("12345678901234567890123");
	test("123456789012345678901234");

	assert(base64::encoded_valid("123") == false);
	assert(base64::encoded_valid("123=") == true);
	//!TODO more test cases to chck for invalid encodings

	//!TODO use catch2 for testing


	return EXIT_SUCCESS;
}
