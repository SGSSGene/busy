#!/bin/bash
mkdir packages
git clone git@cakeface:aBuild/Serializer      packages/Serializer
git clone git@cakeface:aBuild/jsoncpp         packages/jsoncpp
git clone git@cakeface:aBuild/CommonOptions   packages/CommonOptions
git clone git@cakeface:aBuild/ThreadPool      packages/ThreadPool

g++-4.9 -ggdb -O0 --std=c++11 \
	-isystem packages/Serializer/src/ \
	-isystem packages/jsoncpp/include \
	-isystem packages/CommonOptions/src \
	-isystem packages/ThreadPool/src \
	-I src/aBuild/ \
	src/aBuild/*.cpp \
	src/aBuild/commands/*.cpp \
	packages/Serializer/src/serializer/*.cpp \
	packages/jsoncpp/src/lib_json/json_reader.cpp \
	packages/jsoncpp/src/lib_json/json_value.cpp \
	packages/jsoncpp/src/lib_json/json_writer.cpp \
	-lpthread \
	-o aBuild
./aBuild
./aBuild --test
sudo ./aBuild --install
rm ./aBuild
