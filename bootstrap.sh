#!/bin/bash

mkdir -p packages
git clone git@github.com:SGSSGene/Serializer.git     extRepositories/Serializer
git clone git@github.com:SGSSGene/Process.git        extRepositories/Process
git clone git@github.com:SGSSGene/jsoncpp.git        extRepositories/jsoncpp
git clone git@github.com:SGSSGene/CommonOptions.git  extRepositories/CommonOptions
git clone git@github.com:SGSSGene/ThreadPool.git     extRepositories/ThreadPool
git clone git@github.com:SGSSGene/yaml-cpp.git       extRepositories/yaml-cpp

set -e

g++ -ggdb -O0 --std=c++11 \
	-isystem extRepositories/Serializer/src/ \
	-isystem extRepositories/jsoncpp/include \
	-isystem extRepositories/CommonOptions/src \
	-isystem extRepositories/ThreadPool/src \
	-isystem extRepositories/yaml-cpp/include \
	-isystem extRepositories/Process/src \
	-I src/busy/ \
	src/busy/*.cpp \
	src/busy/commands/*.cpp \
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
	-o busy
./busy build busy

echo "run:"
echo "$ sudo ./busy install"
echo "or copy build/system-gcc-4.9/busy to /usr/bin"

