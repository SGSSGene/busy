#!/bin/bash
mkdir packages
git clone git@78.47.95.250:aBuild/JsonSerializer  packages/JsonSerializer
git clone git@78.47.95.250:aBuild/jsoncpp         packages/jsoncpp
clang++ -ggdb -O0 --std=c++11 \
	-isystem packages/JsonSerializer/src/ \
	-isystem packages/jsoncpp/include \
	src/aBuild/*.cpp \
	packages/jsoncpp/src/lib_json/json_reader.cpp \
	packages/jsoncpp/src/lib_json/json_value.cpp \
	packages/jsoncpp/src/lib_json/json_writer.cpp \
	-o aBuild
./aBuild
./aBuild test
sudo ./aBuild install
rm ./aBuild
