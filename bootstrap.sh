#!/bin/bash

mkdir -p packages
git clone git@cakeface.de:aBuild/Serializer      extRepositories/Serializer
git clone git@cakeface.de:aBuild/Process         extRepositories/Process
git clone git@cakeface.de:aBuild/jsoncpp         extRepositories/jsoncpp
git clone git@cakeface.de:aBuild/CommonOptions   extRepositories/CommonOptions
git clone git@cakeface.de:aBuild/ThreadPool      extRepositories/ThreadPool
git clone git@cakeface.de:aBuild/yaml-cpp        extRepositories/yaml-cpp

set -e

g++ -ggdb -O0 --std=c++11 \
	-isystem extRepositories/Serializer/src/ \
	-isystem extRepositories/jsoncpp/include \
	-isystem extRepositories/CommonOptions/src \
	-isystem extRepositories/ThreadPool/src \
	-isystem extRepositories/yaml-cpp/include \
	-isystem extRepositories/Process/src \
	-I src/aBuild/ \
	src/aBuild/*.cpp \
	src/aBuild/commands/*.cpp \
	extRepositories/Serializer/src/serializer/binary/*.cpp \
	extRepositories/Serializer/src/serializer/json/*.cpp \
	extRepositories/Serializer/src/serializer/yaml/*.cpp \
	extRepositories/jsoncpp/src/lib_json/json_reader.cpp \
	extRepositories/jsoncpp/src/lib_json/json_value.cpp \
	extRepositories/jsoncpp/src/lib_json/json_writer.cpp \
	extRepositories/yaml-cpp/src/yaml-cpp/*.cpp \
	extRepositories/CommonOptions/src/commonOptions/*.cpp \
	extRepositories/Process/src/process/*.cpp \
	-lpthread \
	-o aBuild
./aBuild build aBuild

echo "run:"
echo "$ sudo ./aBuild install"
echo "or copy build/system-gcc-4.9/aBuild to /usr/bin"

