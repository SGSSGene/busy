#!/bin/bash

set -Eeuo pipefail
mkdir -p bootstrap.d
cd bootstrap.d

g++ -std=c++20 ../src/busy-desc/main.cpp -lyaml-cpp -lfmt -O0 -ggdb3 -o busy
./busy compile -f ../busy3.yaml -t ../toolchains.d/gcc12.2.sh
