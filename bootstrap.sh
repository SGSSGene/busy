#!/bin/bash

mkdir -p packages
rm -rf extRepositories/Serializer
git clone https://github.com/SGSSGene/Serializer.git     extRepositories/Serializer
rm -rf extRepositories/Process
git clone https://github.com/SGSSGene/Process.git        extRepositories/Process
rm -rf extRepositories/jsoncpp
git clone https://github.com/SGSSGene/jsoncpp.git        extRepositories/jsoncpp
rm -rf extRepositories/CommonOptions
git clone https://github.com/SGSSGene/CommonOptions.git  extRepositories/CommonOptions
rm -rf extRepositories/ThreadPool
git clone https://github.com/SGSSGene/ThreadPool.git     extRepositories/ThreadPool
rm -rf extRepositories/yaml-cpp
git clone https://github.com/SGSSGene/yaml-cpp.git       extRepositories/yaml-cpp

set -e

g++ -ggdb -O0 --std=c++11 \
	-isystem extRepositories/Serializer/src/ \
	-isystem extRepositories/jsoncpp/include \
	-isystem extRepositories/CommonOptions/src \
	-isystem extRepositories/ThreadPool/src \
	-isystem extRepositories/yaml-cpp/include \
	-isystem extRepositories/Process/src \
	-isystem src \
	-I src/busy/ \
	src/busy/*.cpp \
	src/busyConfig/*.cpp \
	src/busyUtils/*.cpp \
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

