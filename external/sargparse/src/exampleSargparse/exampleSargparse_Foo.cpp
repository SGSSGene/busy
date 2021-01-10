#include <sargparse/sargparse.h>
#include <iostream>

namespace {
// all parameters comming from this section have "mySection." as their name prefix
auto mySection = sargp::Section{"mySection"};
// here are some demonstrations of how parameters can be registered
// the arguments passed to Parameters are pretty easy: default_value, argument_name, description_for_help_text
auto myIntParam    = mySection.Parameter<int>(123, "integer", "an integer argument");
auto myDoubleParam = mySection.Parameter<double>(M_PI, "double", "a double argument");
auto myStringParam = mySection.Parameter<std::string>("some string value", "string", "a string argument");
auto myFlag        = mySection.Flag("flag", "a simple flag");

auto myFile        = mySection.Parameter<std::string>("", "file", "a file", []{}, sargp::completeFile());
auto myCppFile     = mySection.Parameter<std::string>("", "cpp_file", "a .cpp file", []{}, sargp::completeFile(".cpp"));
auto myDirectory   = mySection.Parameter<std::string>("", "path", "a path", []{}, sargp::completeDirectory());

auto myMultiFiles       = mySection.Parameter<std::vector<std::string>>({}, "multi_files", "multiple files", []{}, sargp::completeFile("", sargp::File::Multi));
auto myMultiCppFiles    = mySection.Parameter<std::vector<std::string>>({}, "multi_cpp_files", "multiple .cpp files", []{}, sargp::completeFile(".cpp", sargp::File::Multi));
auto myMultiDirectories = mySection.Parameter<std::vector<std::string>>({}, "multi_paths", "multiple paths", []{}, sargp::completeDirectory(sargp::File::Multi));



void myCommandCallback();
// if "my_command" is passed as first argument to the executable myCommandCallback will be called from sargp::callCommands()
auto myCommand = sargp::Command{"my_command", "help text for that command", myCommandCallback};
auto myCommandSpecificParameter = myCommand.Flag("print_hello", "print hello");
auto myTextToPrint = myCommand.Parameter<std::vector<std::string>>({"some", "words"}, "words_to_print", "print some words");

void myCommandCallback() {
	std::cout << "executing \"my_command\"" << std::endl;
	if (myCommandSpecificParameter) {
		std::cout << "hello" << std::endl;
	}
	// if the compiler can infer a cast to the underlying type of the parameter it will do so
	// also: explicit casts are possible to get the value of a parameter or simply use .get()
	for (auto const& word : myTextToPrint.get()) {
		std::cout << word << " ";
	}
	std::cout << std::endl;
}

// choices (e.g., for enums) are also possible
enum class MyEnumType {Foo, Bar};
auto myChoice = sargp::Choice<MyEnumType>{MyEnumType::Foo, "my_enum",
	{{"Foo", MyEnumType::Foo}, {"Bar", MyEnumType::Bar}}, "a choice demonstration"
};

enum class MyEnumType2 {Foo, Bar};
auto myChoice2 = sargp::Parameter<MyEnumType2>{MyEnumType2::Foo, "my_enum2", "a demonstration of a enum without choice"};



}
