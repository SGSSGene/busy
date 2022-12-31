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

    busy compile -f ${project}/busy.yaml -t gcc12.2

    str="$(bin/app)";
    if [ "${str}" != "Hello World" ]; then
        echo "failed 1"
        exit 1
    fi
    cd ..
    rm -rf ${build_path}
)

# check normal compilation of executable and libraries works
(
    build_path="test-build"
    project="../libraryPlusApp"
    rm -rf ${build_path}
    mkdir -p ${build_path}
    cd ${build_path}

    busy compile -f ${project}/busy.yaml -t gcc12.2

    str="$(bin/app)";
    if [ "${str}" != "Hello World" ]; then
        echo "failed 2"
        exit 1
    fi
    cd ..
    rm -rf ${build_path}
)


# check if installation of executable and libraries work
(
    build_path="test-build"
    project="../libraryPlusApp"
    rm -rf ${build_path}
    mkdir -p ${build_path}
    cd ${build_path}

    busy compile -f ${project}/busy.yaml -t gcc12.2
    busy install --prefix fake-root

    if [ ! -f "fake-root/bin/app" ] || [ ! -f "fake-root/lib/libmylib.a" ] \
       || [ ! -f "fake-root/include/mylib/f.h" ] \
       || [ ! -f "fake-root/share/busy/mylib.yaml" ]; then
        echo "failed 3"
        exit 1
    fi
    cd ..
    rm -rf ${build_path}
)


# check compilation fail
(
    build_path="test-build"
    project="../brokenSingleApp"
    rm -rf ${build_path}
    mkdir -p ${build_path}
    cd ${build_path}

    str="$(busy compile -f ${project}/busy.yaml -t gcc12.2 || true)"
    exp=$(cat <<-END
compile error: error compiling:
[01m[Kenvironments/app/src/app/main.cpp:[m[K In function ‘[01m[Kint main()[m[K’:
[01m[Kenvironments/app/src/app/main.cpp:4:5:[m[K [01;31m[Kerror: [m[K‘[01m[Kblub[m[K’ was not declared in this scope
    4 |     [01;31m[Kblub[m[K; // Some invalid code
      |     [01;31m[K^~~~[m[K
END
)

    if [ "${str}" != "${exp}" ]; then
        echo "${str}"
        echo "${exp}"
        echo $ret
        echo "failed 4"
        exit 1
    fi
    cd ..
    rm -rf ${build_path}
)

# check linkage fail
(
    build_path="test-build"
    project="../brokenLinkageApp"
    rm -rf ${build_path}
    mkdir -p ${build_path}
    cd ${build_path}

    str="$(busy compile -f ${project}/busy.yaml -t gcc12.2 2>&1 || true)"

    if [ -z "${str}" ]; then
        echo "${str}"
        echo "failed 5"
        exit 1
    fi
    cd ..
    rm -rf ${build_path}
)

echo Success
