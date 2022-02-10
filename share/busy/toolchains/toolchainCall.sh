#!/bin/bash

# $ ./toolchainCall.sh compile input.cpp output.o -I <includes>... -isystem <system includes>...
# $ ./toolchainCall.sh link static_library output.exe|output.a -i obj1.o obj2.o lib2.a -l pthread armadillo
#
# Return values:
# 0 on success
# 1 on nothing done (e.g. is header, or header only)
# -1 error


function parseMode {
    var="$2"
    if [ "${1}" = "${3}" ]; then
        parseMode="${var}"
        eval ${var}="${!var:-()}"
        r="1"
    fi
}
function parseValue {
    if [ -n "${parseMode}" ]; then
        eval "${parseMode}+=(${1})"
        r="1"
    fi
}
function parse {
    parsePairs=()
    while [ "$1" != "--" ]; do
        parsePairs+=("$1")
        shift
    done
    while [ $# -gt 0 ]; do
        shift
        r=
        for x in "${parsePairs[@]}"; do
            if [ -z "$r" ]; then
                parseMode $x "$1"
            fi
        done
        if [ -z "$r" ]; then
            parseValue "$1"
        fi
    done
}
function implode {
    sep="$1"
    shift;
    for i in "$@"; do
        echo -n "$sep$i"
    done
}

CXX=g++
C=gcc
AR=ar
CHECK_CCACHE=1

version=$(${C} --version | head -n 1 | perl -ne "s/\([^\)]*\)/()/g;print" | cut -d " " -f 3)

version_major=$(echo ${version} | cut -d "." -f 1)
version_minor=$(echo ${version} | cut -d "." -f 2)
version_patch=$(echo ${version} | cut -d "." -f 3)


# check if version numbers match
if [ "${version_major}" != "9" ] || [ "${version_minor}" != "1" ]; then
    exit -1
fi

# todo check if platform macthes

# check if ccache is available
which ccache > /dev/null 2>&1
if [ "$?" != "0" ] && [ "${CHECK_CCACHE}" ]; then
    exit -1
fi

if [ "${CHECK_CCACHE}" ]; then
    CXX="ccache ${CXX}"
    C="ccache ${C}"
    AR="ccache ${AR}"
fi


if [ "$1" == "info" ]; then
shift

cat <<-END
toolchains:
  - name: "gcc 9.1"
    version: ${version}
    detail: "$(${CXX} --version | head -1)"
    which:
      - "${CXX}"
      - "${C}"
      - "${AR}"
    options:
      - "release"
      - "release_with_symbols"
      - "debug"
END
exit 0
fi


if [ $# -lt 5 ]; then
    if [ "$1" == "begin" ]; then
        if [ ! -e "external" ]; then
            ln -s ../external external
        fi
        if [ ! -e "src" ]; then
            ln -s ../src src
        fi
        exit 0
    elif [ "$1" == "end" ]; then
        exit 0
    fi
    exit -1;
fi

if [ "$1" == "compile" ]; then
    shift; inputFile="$1"
    shift; outputFile="$1"
    shift

    parse "-ilocal  projectIncludes" \
          "-isystem systemIncludes" \
          "--" "$@"

    projectIncludes=$(implode " -I " "${projectIncludes[@]}")
    systemIncludes=$(implode " -isystem " "${systemIncludes[@]}")

    filetype="$(echo "${inputFile}" | rev | cut -d "." -f 1 | rev)";
    if [ "${filetype}" = "cpp" ]; then
        call="${CXX} -O0 -std=c++17 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c $inputFile -o $outputFile $projectIncludes $systemIncludes"
    elif [ "${filetype}" = "c" ]; then
        call="${C} -O0 -std=c11 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c $inputFile -o $outputFile $projectIncludes $systemIncludes"
    else
        exit 1
    fi
elif [ "$1" == "link" ]; then
    shift; target="$1"
    shift; outputFile="$1"
    shift

    parse "-i       inputFiles" \
          "-il      inputLibraries" \
          "-l       libraries" \
          "--" "$@"

    libraries=($(implode " -l" "${libraries[@]}"))

    # Header only
    if [ "${#inputFiles[@]}" -eq 0 ]; then
        exit 1
    # Executable
    elif [ "${target}" == "executable" ]; then
        call="${CXX} -rdynamic -g3 -ggdb -fdiagnostics-color=always -o $outputFile ${inputFiles[@]} ${inputLibraries[@]} ${libraries[@]}"

    # Static library?
    elif [ "${target}" == "static_library" ]; then
        call="${AR} rcs $outputFile ${inputFiles[@]}"
    else
        exit -1
    fi
else
    exit -1
fi

mkdir -p $(dirname ${outputFile})
$call 1>stdout.log 2>stderr.log
errorCode=$?

echo "stdout: |+"
cat stdout.log | sed 's/^/    /'
echo "stderr: |+"
cat stderr.log | sed 's/^/    /'

rm stdout.log
rm stderr.log
if [ $errorCode -ne "0" ]; then
    exit -1
fi
exit 0
