#!/bin/bash

set -Eeuo pipefail
mkdir -p bootstrap.d
cd bootstrap.d

g++ -std=c++20 ../src/busy/main.cpp -lyaml-cpp -lfmt -O0 -ggdb3 -o busy
export BUSY_ROOT=../share/busy/fake-root/
./busy compile -f ../busy.yaml -t gcc12.2
rm busy
echo "Compiling busy was a success!"
echo ""
echo "add following line to your .bashrc or .zshrc:"
echo "        "'export PATH="${HOME}/.config/busy/env/bin:${PATH+:$PATH}"'
echo ""
echo "and install busy by calling"
echo "        (cd bootstrap.d; ./bin/busy install)"
