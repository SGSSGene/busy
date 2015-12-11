#!/bin/bash

mkdir -p packages
git clone git@cakeface.de:aBuild/Serializer      packages/Serializer
git clone git@cakeface.de:aBuild/jsoncpp         packages/jsoncpp
git clone git@cakeface.de:aBuild/CommonOptions   packages/CommonOptions
git clone git@cakeface.de:aBuild/ThreadPool      packages/ThreadPool

set -e

g++ -ggdb -O0 --std=c++11 \
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
sudo ./aBuild --install
rm ./aBuild
