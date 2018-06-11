#!/bin/bash

cd ~/src
export PATH="/usr/lib/ccache/bin:${PATH}"
./bootstrap.sh

rm -rf root
mkdir -p root/usr/bin
mkdir -p root/usr/share/busy

cp share/busy/completions/bash_completion root/usr/share/busy/busy
cp build/fallback-gcc/debug/busy root/usr/bin/busy
