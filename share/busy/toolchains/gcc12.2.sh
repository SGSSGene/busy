#!/bin/bash

# $ <$0> info
# $ <$0> compile input.cpp output.o -ilocal <includes>... -isystem <system includes>...
# $ <$0> link static_library output.a -i obj1.o obj2.o lib2.a -l pthread armadillo
# $ <$0> link executable output.exe -i obj1.o obj2.o lib2.a -l pthread armadillo

# Return values:
# 0 on success
# -1 error

BUSY_TOOLCHAIN_CALL=1
NAME="gcc 12.2"
CXX=/usr/bin/g++
C=/usr/bin/gcc
LD=/usr/bin/ld
AR=/usr/bin/ar
CXX_STD="-std=c++20"
C_STD="-std=c18"
BUILD_SCRIPTS="${0}"
REAL_CALL="${@}"

expected_major=12
expected_minor=2

declare -A profiles
profiles=(
    ["default"]="debug ccache no-default"
    ["native"]="no-release no-release_with_symbols no-debug"
    ["release"]="no-debug no-release_with_symbols no-native"
    ["release_with_symbols"]="no-release no-debug no-native"
    ["debug"]="no-release no-release_with_symbols no-native"
    ["profile"]=""
    ["strict"]=""
    ["ccache"]=""
)


declare -A profile_compile_param
profile_compile_param=(
    ["native"]=" -O3 -march=native "
    ["release"]=" -O3"
    ["release_with_symbols"]=" -O3 -ggdb"
    ["debug"]=" -O0 -ggdb"
    ["profile"]=" -fprofile-arcs -ftest-coverage -fPIC"
    ["strict"]=" -Wall -Wextra -Wpedantic"
)

declare -A profile_link_param
profile_link_param=(
    ["release_with_symbols"]=" -g3 -ggdb"
    ["debug"]=" -g3 -ggdb"
    ["profile"]=" -lgcov"
)
declare -A extraPackages
extraPackages=(
    "stdlib"
)

source "${0%/*}"/gcc_base.sh
