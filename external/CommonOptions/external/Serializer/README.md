\mainpage
# JsonSerializer
Classes and interfaces to easily serialize objects into json objects.
One function for serialization and deserialization. Different function for serialization/deserialization possible if needed.

## Dependencies
* Uses https://github.com/open-source-parsers/jsoncpp
* Uses https://code.google.com/p/googletest/
* aBuild support
* support for genericFactory (optional)
* support for armadillo  (optional)

## Support
Default supported datatypes:
* bool
* uint8_t/int8_t
* uint16_t/int16_t
* uint32_t/int32_t
* float/double
* std::string
* std::array
* std::vector
* std::list
* std::map
* std::unique_ptr (genericFactory support optional)
* std::shared_ptr (genericFactory support optional)
* arma::mat (optional)
* arma::vec (optional)
* arma::colvec (optional)
* arma::rowvec (optional)
* extendable for other data types

## Example
* \ref src/testJsonSerializer/main.cpp "generic example"
* \ref src/testJsonSerializerPointer/main.cpp "using smart pointers"
* \ref src/testJsonSerializerGenericFactory/main.cpp "using Genericfactory"
* \ref src/testJsonSerializerArmadillo/main.cpp "using Armadillo"

### Function declaration

	#include <jsonSerializer/jsonSerializer.h>
	class A {
	private:
		std::string name;
		uint32_t x;
		std::vector<double> values;
	public:
		void serialize(jsonSerializer::Node& node) {
			node["name"]   % name or "unknown"; //(de-)serialize name, if name is not set, set it to "unknown"
			node["x"]      % x;
			node["values"] % values;
		}
	};

	int main() {
		A a1, a2;
		jsonSerializer::write("file.json", a1);
		jsonSerializer::read("file.json", a2);
		// a1 and a2 are now the same

		std::vector<double> vec{1, 2, 3, 4};
		jsonSerializer::write("file2.json", vec);
		return 0;
	}

