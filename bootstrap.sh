#!/bin/bash
mkdir packages
git clone ../JsonSerializer.git -b aBuild packages/JsonSerializer
git clone ../jsoncpp.git        -b aBuild packages/jsoncpp
clang++ -ggdb -O0 --std=c++11 \
	-isystem packages/JsonSerializer/src/ \
	-isystem packages/jsoncpp/include \
	src/aBuild/*.cpp \
	packages/jsoncpp/src/lib_json/json_reader.cpp \
	packages/jsoncpp/src/lib_json/json_value.cpp \
	packages/jsoncpp/src/lib_json/json_writer.cpp \
	-o aBuild
./aBuild
sudo ./aBuild install
rm ./aBuild
