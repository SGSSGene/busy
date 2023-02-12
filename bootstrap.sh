#!/bin/bash

# chaneg into script directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "$DIR"

install=""
if [ "$1" == "--install" ]; then
    install="y"
fi

set -Eeuo pipefail
mkdir -p bootstrap.d
cd bootstrap.d

g++ -std=c++20 ../src/busy/main.cpp -lyaml-cpp -lfmt -O0 -ggdb3 -o busy
export BUSY_ROOT=../share/busy/fake-root/
./busy compile -f ../busy.yaml -t gcc12.2
rm busy
if [ -n "${install}" ]; then
    ./bin/busy install
fi
echo "Compiling busy was a success!"
echo ""
echo "add following line to your .bashrc or .zshrc:"
echo "        "'export PATH="${HOME}/.config/busy/env/bin:${PATH+:$PATH}"'
if [ -z "${install}" ]; then
    echo ""
    echo "and install busy by calling"
    echo "        bootstrap.sh --install"
fi
