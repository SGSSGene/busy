#include <clice/clice.h>

#include <iostream>


namespace {
auto cliHelp    = clice::Argument{ .arg    = {"--help"},
                                   .desc   = "prints the help page",
                                   .cb     = []{ std::cout << clice::generateHelp(); exit(0); },
                                 };

auto cliAdd     = clice::Argument{ .arg    = "add",
                                   .desc   = "adds some stuff",
                                 };
auto cliVerbose = clice::Argument{ .parent = &cliAdd,
                                   .arg    = "--verbose",
                                   .desc   = "detailed description of what is happening",
                                  };
auto cliNbr     = clice::Argument{ .arg    = "--nbr",
                                   .desc   = "setting some nbr",
                                   .value  = 5,
                                 };
auto cliInts    = clice::Argument{ .arg    = "--ints",
                                   .desc   = "a list of numbers",
                                   .value  = std::vector<int>{},
                                 };
auto cliAuto    = clice::Argument{ .arg    = "--auto",
                                   .desc   = "value depending on cliNbr +1",
                                   .value  = []() { return *cliNbr+1; },
                                   .cb     = []() {
                                        std::cout << "done\n";
                                   }
                                 };
auto cliBasicTypes = clice::Argument{ .arg  = "basic",
                                      .desc = "basic values like bool, ints and doubles",
                                    };
auto cliBTFlag     = clice::Argument{ .parent = &cliBasicTypes,
                                      .arg    = "--flag",
                                      .desc   = "flag value",
                                    };
auto cliBTBool     = clice::Argument{ .parent = &cliBasicTypes,
                                      .arg    = "--bool",
                                      .desc   = "bool value",
                                      .value  = false,
                                    };
auto cliBTUInt8   = clice::Argument{ .parent = &cliBasicTypes,
                                     .arg    = "--uint8",
                                     .desc   = "uint8 value",
                                     .value  = uint8_t{},
                                   };
auto cliBTInt8    = clice::Argument{ .parent = &cliBasicTypes,
                                     .arg    = "--int8",
                                     .desc   = "int8 value",
                                     .value  = int8_t{},
                                   };
auto cliBTUInt16   = clice::Argument{ .parent = &cliBasicTypes,
                                     .arg    = "--uint16",
                                     .desc   = "uint16 value",
                                     .value  = uint16_t{},
                                   };
auto cliBTInt16    = clice::Argument{ .parent = &cliBasicTypes,
                                     .arg    = "--int16",
                                     .desc   = "int16 value",
                                     .value  = int16_t{},
                                   };
auto cliBTUInt32   = clice::Argument{ .parent = &cliBasicTypes,
                                     .arg    = "--uint32",
                                     .desc   = "uint32 value",
                                     .value  = uint32_t{},
                                   };
auto cliBTInt32    = clice::Argument{ .parent = &cliBasicTypes,
                                     .arg    = "--int32",
                                     .desc   = "int32 value",
                                     .value  = int32_t{},
                                   };
auto cliBTUInt64   = clice::Argument{ .parent = &cliBasicTypes,
                                     .arg    = "--uint64",
                                     .desc   = "uint64 value",
                                     .value  = uint64_t{},
                                   };
auto cliBTInt64    = clice::Argument{ .parent = &cliBasicTypes,
                                     .arg    = "--int64",
                                     .desc   = "int64 value",
                                     .value  = int64_t{},
                                    };
auto cliBTChar    = clice::Argument{ .parent = &cliBasicTypes,
                                     .arg    = "--char",
                                     .desc   = "char value",
                                     .value  = char{'A'},
                                    };
auto cliBTString  = clice::Argument{ .parent = &cliBasicTypes,
                                     .arg    = "--string",
                                     .desc   = "string value",
                                     .value  = std::string{},
                                    };
auto cliBTMappedBool = clice::Argument{ .parent = &cliBasicTypes,
                                     .arg    = "--mapped_bool",
                                     .desc   = "takes \"good\" and \"bad\" as input",
                                     .value  = bool{},
                                     .mapping = {{{"good", true}, {"bad", false}}},
                                    };

enum class MyEnumType {Foo, Bar};
auto cliBTEnum = clice::Argument{ .parent = &cliBasicTypes,
                                  .arg    = "--enum",
                                  .desc   = "takes \"foo\" and \"bar\" as input",
                                  .value  = MyEnumType::Foo,
                                  .mapping = {{{"foo", MyEnumType::Foo}, {"bar", MyEnumType::Bar}}},
                                 };

auto cliBTInputPath = clice::Argument{ .parent = &cliBasicTypes,
                                  .arg    = "--input",
                                  .desc   = "input path",
                                  .value  = std::filesystem::path{},
                                 };
auto cliBTOutputPath = clice::Argument{ .parent = &cliBasicTypes,
                                  .arg    = "--output",
                                  .desc   = "output path",
                                  .value  = std::filesystem::path{},
                                 };
auto cliBTVectorInt = clice::Argument{ .parent = &cliBasicTypes,
                                       .arg    = "--vector_int",
                                       .desc   = "vector of ints",
                                       .value  = std::vector<int>{},
                                     };
}

int main(int argc, char** argv) {
    if (auto failed = clice::parse(argc, argv); failed) {
        std::cerr << "parsing failed: " << *failed << "\n";
        return -1;
    }
    if (auto ptr = std::getenv("CLICE_COMPLETION"); ptr) {
        return 0;
    }


    std::cout << cliAdd << "\n";
    std::cout << "  " << cliVerbose << "\n";
    std::cout << cliHelp << "\n";
    std::cout << *cliNbr << "\n";
    std::cout << cliInts << "\n";
    for (auto i : *cliInts) {
        std::cout << " - " << i << "\n";
    }
    std::cout << "auto: " << *cliAuto << "\n";

    std::cout << "\n\nBasic Types: " << cliBasicTypes << "\n";
    std::cout << "  --flag " << cliBTFlag << " " << "\n";
    std::cout << "  --bool " << cliBTBool << " " << *cliBTBool << "\n";
    std::cout << "  --uint8 " << cliBTUInt8 << " " << (int)*cliBTUInt8 << "\n";
    std::cout << "  --int8 " << cliBTInt8 << " " << (int)*cliBTInt8 << "\n";
    std::cout << "  --uint16 " << cliBTUInt16 << " " << *cliBTUInt16 << "\n";
    std::cout << "  --int16 " << cliBTInt16 << " " << *cliBTInt16 << "\n";
    std::cout << "  --uint32 " << cliBTUInt32 << " " << *cliBTUInt32 << "\n";
    std::cout << "  --int32 " << cliBTInt32 << " " << *cliBTInt32 << "\n";
    std::cout << "  --uint64 " << cliBTUInt64 << " " << *cliBTUInt64 << "\n";
    std::cout << "  --int64 " << cliBTInt64 << " " << *cliBTInt64 << "\n";
    std::cout << "  --char " << cliBTChar << " " << *cliBTChar << "\n";
    std::cout << "  --string " << cliBTString << " " << *cliBTString << "\n";
    std::cout << "  --mapped_bool " << cliBTMappedBool << " " << *cliBTMappedBool << "\n";
    std::cout << "  --enum " << cliBTEnum << " " << (*cliBTEnum==MyEnumType::Foo?"foo":"bar") << "\n";
    std::cout << "  --input " << cliBTInputPath << " " << *cliBTInputPath << "\n";
    std::cout << "  --output " << cliBTOutputPath << " " << *cliBTOutputPath << "\n";
    std::cout << "  --vector_int " << cliBTVectorInt << " " << (*cliBTVectorInt).size() << "\n";
    for (auto x : *cliBTVectorInt) {
        std::cout << "    " << x << "\n";
    }

}
