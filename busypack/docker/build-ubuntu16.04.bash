#!/bin/bash

cd ~/src
export PATH="/usr/lib/ccache/:${PATH}"
./bootstrap.sh

rm -rf root
mkdir -p root/usr/bin
mkdir -p root/usr/share/busy

cp share/busy/completions/bash_completion root/etc/bash_completion.d/busy
cp build/fallback-gcc/debug/busy root/usr/bin/busy
