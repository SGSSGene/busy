#!/bin/bash

# change into script directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
cd "$DIR"

install=""
if [ "$1" == "--install" ]; then
    install="y"
elif [ -n "$1" ]; then
    echo "unknown argument $1"
    exit 255
fi

set -Eeuo pipefail
mkdir -p bootstrap.d && cd $_

echo "building temporary busy executable"
g++ -std=c++20 -lyaml-cpp -lfmt -O0 -ggdb3 -o busy \
    -isystem ../src \
    ../src/busy/main.cpp \
    ../src/busy-lib/main.cpp \
    ../src/busy-lib/cmdInstall.cpp \
    ../src/busy-lib/cmdStatus.cpp \
    ../src/busy-lib/cmdInfo.cpp \
    ../src/clice-main/main.cpp \
    ../src/clice/Argument.cpp



echo "creating proper busy environment"
mkdir -p tmp-root

for f in "compilers" "stdlib" "yaml-cpp" "fmt"; do
    rm -rf build
    (
        mkdir build && cd $_
        ../busy compile -f ../../extern/$f.yaml
        ../busy install --prefix ../tmp-root
        if [ -n "${install}" ]; then
            ../busy install
        fi
    )
done

echo "compiling busy with busy"
export BUSY_ROOT=tmp-root
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
