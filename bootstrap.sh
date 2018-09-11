#!/bin/bash

rm -rf .busy
rm -rf ./build

set -e

mkdir -p busy-helper/busy-version
echo "
#pragma once
#define VERSION_BUSY          \"bootstrap-build\"
#define VERSION_COMMONOPTIONS \"bootstrap-build\"
#define VERSION_PROCESS       \"bootstrap-build\"
#define VERSION_SELFTEST      \"bootstrap-build\"
#define VERSION_SERIALIZER    \"bootstrap-build\"
#define VERSION_THREADPOOL    \"bootstrap-build\"
#define VERSION_GTEST         \"bootstrap-build\"
#define VERSION_JSONCPP       \"bootstrap-build\"
#define VERSION_YAML-CPP      \"bootstrap-build\"
" > busy-helper/busy-version/version.h

echo "initial build, this will take a few minutes"
g++ -ggdb -O0 --std=c++11 \
	-isystem extRepositories/CommonOptions/extRepositories/Serializer/src/ \
	-isystem extRepositories/CommonOptions/extRepositories/Serializer/extRepositories/jsoncpp/include \
	-isystem extRepositories/CommonOptions/src \
	-isystem extRepositories/ThreadPool/src \
	-isystem extRepositories/CommonOptions/extRepositories/Serializer/extRepositories/yaml-cpp/include \
	-isystem extRepositories/CommonOptions/extRepositories/SelfTest/extRepositories/Process/src \
	-isystem busy-helper \
	-isystem src \
	-I src/busy/ \
	src/busy/*.cpp \
	src/busy/analyse/*.cpp \
	src/busy/commands/*.cpp \
	src/busyConfig/*.cpp \
	src/busyUtils/*.cpp \
	src/busyVersion/*.cpp \
	extRepositories/CommonOptions/extRepositories/Serializer/src/serializer/binary/*.cpp \
	extRepositories/CommonOptions/extRepositories/Serializer/src/serializer/json/*.cpp \
	extRepositories/CommonOptions/extRepositories/Serializer/src/serializer/yaml/*.cpp \
	extRepositories/CommonOptions/extRepositories/Serializer/extRepositories/jsoncpp/src/lib_json/json_reader.cpp \
	extRepositories/CommonOptions/extRepositories/Serializer/extRepositories/jsoncpp/src/lib_json/json_value.cpp \
	extRepositories/CommonOptions/extRepositories/Serializer/extRepositories/jsoncpp/src/lib_json/json_writer.cpp \
	extRepositories/CommonOptions/extRepositories/Serializer/extRepositories/yaml-cpp/src/yaml-cpp/*.cpp \
	extRepositories/CommonOptions/src/commonOptions/*.cpp \
	extRepositories/CommonOptions/extRepositories/SelfTest/extRepositories/Process/src/process/*.cpp \
	-lpthread \
	-o busy
rm -rf busy-helper
echo "rebuild busy with busy"
./busy build busy

echo "run:"
echo "copy build/fallback-gcc/debug/busy to /usr/bin"

