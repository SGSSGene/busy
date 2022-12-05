#!/bin/bash
#
cd "${0%/*}"

set -Eeuo pipefail


# check normal compilation of executable works
(
    build_path="test-build"
    project="../singleApp"
    rm -rf ${build_path}
    mkdir -p ${build_path}
    cd ${build_path}

    busy compile -f ${project}/busy.yaml -t ../../toolchains.d/gcc12.2.sh

    if [ "$(bin/app)" != "Hello World" ]; then
        echo "failed"
        exit 1
    fi
    cd ..
    rm -rf ${build_path}
)

# check normal compilation of executable works
(
    build_path="test-build"
    project="../libraryPlusApp"
    rm -rf ${build_path}
    mkdir -p ${build_path}
    cd ${build_path}

    busy compile -f ${project}/busy.yaml -t ../../toolchains.d/gcc12.2.sh

    if [ "$(bin/app)" != "Hello World" ]; then
        echo "failed"
        exit 1
    fi
    cd ..
    rm -rf ${build_path}
)
