#!/bin/bash

# $ <$0> info
# $ <$0> compile input.cpp output.o -ilocal <includes>... -isystem <system includes>...
# $ <$0> link static_library output.a -i obj1.o obj2.o lib2.a -l pthread armadillo
# $ <$0> link executable output.exe -i obj1.o obj2.o lib2.a -l pthread armadillo

# Return values:
# 0 on success
# -1 error

BUSY_TOOLCHAIN_CALL=1
NAME="gcc 9.3"
CXX=/usr/bin/g++-9
C=/usr/bin/gcc-9
LD=/usr/bin/ld
AR=/usr/bin/ar

CXX_STD="-std=c++2a"
C_STD="-std=c18"


expected_major=9
expected_minor_ge=3

declare -A profiles
profiles=(
    ["default"]="debug ccache no-default"
    ["release"]="no_debug no-release no-release_with_symbols"
    ["release_with_symbols"]="no-release no-debug"
    ["debug"]="no-release no-release_with_symbols"
    ["profile"]=""
    ["strict"]=""
    ["ccache"]=""
)


declare -A profile_compile_param
profile_compile_param=(
    ["release"]=" -O2"
    ["release_with_symbols"]=" -O2 -ggdb"
    ["debug"]=" -O0 -ggdb"
    ["profile"]=" -fprofile-arcs -ftest-coverage -fPIC"
    ["strict"]=" -Wall -Wextra -Wpedantic"
)

declare -A profile_link_param
profile_link_param=(
    ["release_with_symbols"]=" -g3 -ggdb"
    ["debug"]=" -g3 -ggdb"
)
declare -A profile_link_libraries
profile_link_libraries=(
    ["profile"]=" gcov"
)

source "${0%/*}"/gcc_base.sh
