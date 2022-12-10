#!/bin/bash

set -Eeuo pipefail
mkdir -p bootstrap.d
cd bootstrap.d

g++ -std=c++20 ../src/busy/main.cpp -lyaml-cpp -lfmt -O0 -ggdb3 -o busy
export BUSY_ROOT=../share/busy/fake-root/
./busy compile -f ../busy.yaml -t gcc12.2
