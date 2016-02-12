#!/bin/bash

mkdir -p packages
git clone git@cakeface.de:aBuild/Serializer      packages/Serializer
git clone git@cakeface.de:aBuild/Process         packages/Process
git clone git@cakeface.de:aBuild/jsoncpp         packages/jsoncpp
git clone git@cakeface.de:aBuild/CommonOptions   packages/CommonOptions
git clone git@cakeface.de:aBuild/ThreadPool      packages/ThreadPool
git clone git@cakeface.de:aBuild/yaml-cpp        packages/yaml-cpp

set -e

g++ -ggdb -O0 --std=c++11 \
	-isystem packages/Serializer/src/ \
	-isystem packages/jsoncpp/include \
	-isystem packages/CommonOptions/src \
	-isystem packages/ThreadPool/src \
	-isystem packages/yaml-cpp/include \
	-isystem packages/Process/src \
	-I src/aBuild/ \
	src/aBuild/*.cpp \
	src/aBuild/commands/*.cpp \
	packages/Serializer/src/serializer/binary/*.cpp \
	packages/Serializer/src/serializer/json/*.cpp \
	packages/Serializer/src/serializer/yaml/*.cpp \
	packages/jsoncpp/src/lib_json/json_reader.cpp \
	packages/jsoncpp/src/lib_json/json_value.cpp \
	packages/jsoncpp/src/lib_json/json_writer.cpp \
	packages/yaml-cpp/src/yaml-cpp/*.cpp \
	packages/CommonOptions/src/commonOptions/*.cpp \
	-lpthread \
	-o aBuild
./aBuild --build aBuild
sudo ./aBuild --install
rm ./aBuild
