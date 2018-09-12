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
g++ -ggdb -O0 --std=c++17 \
	-isystem external/CommonOptions/external/Serializer/src/ \
	-isystem external/CommonOptions/external/Serializer/external/jsoncpp/include \
	-isystem external/CommonOptions/src \
	-isystem external/ThreadPool/src \
	-isystem external/CommonOptions/external/Serializer/external/yaml-cpp/include \
	-isystem external/CommonOptions/external/SelfTest/external/Process/src \
	-isystem busy-helper \
	-isystem src \
	-I src/busy/ \
	src/busy/*.cpp \
	src/busy/analyse/*.cpp \
	src/busy/commands/*.cpp \
	src/busyConfig/*.cpp \
	src/busyUtils/*.cpp \
	src/busyVersion/*.cpp \
	external/CommonOptions/external/Serializer/src/serializer/binary/*.cpp \
	external/CommonOptions/external/Serializer/src/serializer/json/*.cpp \
	external/CommonOptions/external/Serializer/src/serializer/yaml/*.cpp \
	external/CommonOptions/external/Serializer/external/jsoncpp/src/lib_json/json_reader.cpp \
	external/CommonOptions/external/Serializer/external/jsoncpp/src/lib_json/json_value.cpp \
	external/CommonOptions/external/Serializer/external/jsoncpp/src/lib_json/json_writer.cpp \
	external/CommonOptions/external/Serializer/external/yaml-cpp/src/yaml-cpp/*.cpp \
	external/CommonOptions/src/commonOptions/*.cpp \
	external/CommonOptions/external/SelfTest/external/Process/src/process/*.cpp \
	-lpthread \
	-o busy
rm -rf busy-helper
echo "rebuild busy with busy"
./busy build busy

echo "run:"
echo "copy build/fallback-gcc/debug/busy to /usr/bin"

