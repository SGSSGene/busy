#!/bin/bash

# $ <$0> info
# $ <$0> compile input.cpp output.o -ilocal <includes>... -isystem <system includes>...
# $ <$0> link static_library output.a -i obj1.o obj2.o lib2.a -l pthread armadillo
# $ <$0> link executable output.exe -i obj1.o obj2.o lib2.a -l pthread armadillo

# Return values:
# 0 on success
# -1 error

source "${0%/*}"/helper_utils.sh

CXX=/usr/bin/clang-tidy
C=/usr/bin/clang-tidy
LD=/usr/bin/ld
AR=/usr/bin/ar

version_detailed="$(${C} --version | head -n 2 || tail -n 1)"
version=$(echo ${version_detailed} | perl -ne "s/\([^\)]*\)/()/g;print" | cut -d " " -f 5)

version_major=$(echo ${version} | cut -d "." -f 1)
version_minor=$(echo ${version} | cut -d "." -f 2)
version_patch=$(echo ${version} | cut -d "." -f 3)


# check if version numbers match
if [ "${version_major}" != "10" ] || [ "${version_minor}" -lt "0" ]; then
    exit -1
fi

parse "--options    options"\
      "--verbose    verbose"\
      "--" "$@"

if [ "$1" == "info" ]; then
shift

cat <<-END
toolchains:
  - name: "clang-tidy"
    version: ${version}
    detail: "$(${CXX} --version | head -n 2 | tail -n 1)"
    which:
      - "${CXX}"
      - "${C}"
      - "${AR}"
      - "${LD}"
    options:
      default: []
END
exit 0
fi


if [ "$1" == "begin" ]; then
    rootDir="$2"
    if [ ! -e "external" ]; then
        ln -s ${rootDir}/external external
    fi
    if [ ! -e "src" ]; then
        ln -s ${rootDir}/src src
    fi
    hash="$(echo "${@}" | cat - ${0} ${0%/*}/helper_utils.sh ${CXX} ${C} ${LD} ${AR} | shasum)"
    echo "hash: ${hash}";
    exit 0
elif [ "$1" == "end" ]; then
    exit 0
elif [ "$1" == "compile" ]; then
    shift; inputFile="$1"
    shift; outputFile="$1"
    shift

    dependencyFile=$(echo "${outputFile}" | rev | cut -b 3- | rev | xargs -I '{}' echo '{}.d')
    stdoutFile="${outputFile}.stdout"
    stderrFile="${outputFile}.stderr"

    parse "--ilocal  projectIncludes" \
          "--isystem systemIncludes" \
          "--" "$@"

    projectIncludes+=($(dirname ${projectIncludes[-1]})) #!TODO this line should not be needed
    projectIncludes=$(implode " -I " "${projectIncludes[@]}")
    systemIncludes=$(implode " -isystem " "${systemIncludes[@]}")

    filetype="$(echo "${inputFile}" | rev | cut -d "." -f 1 | rev)";
    checks='-header-filter=".*" -checks="-*,clang-analyzer-*,performance-*,modernize-*,bugprone-*"'
    compiler_params="-c ${inputFile} -o ${outputFile} $projectIncludes $systemIncludes"
    if [[ "${filetype}" =~ ^(cpp|cc)$ ]]; then
        call="clang -E -MD -std=c++20 ${compiler_params} && ${CXX} ${inputFile} ${checks} -- -std=c++20 ${compiler_params}"
    elif [ "${filetype}" = "c" ]; then
        call="clang -E -MD -std=c18 ${compiler_params} && ${C} ${inputFile} ${checks} -- -std=c18 ${compiler_params}"
    else
        exit 0
    fi
elif [ "$1" == "link" ]; then
    echo "compilable: false"
    exit 0
else
    exit -1
fi

mkdir -p $(dirname ${outputFile})
: > ${stdoutFile}
if [ -n "${set_verbose}" ]; then
    echo $call>>${stdoutFile}
fi
eval $call 2>>${stdoutFile} 1>${stderrFile}
#eval $call 1>>${stdoutFile} 2>/dev/null

errorCode=$?


echo "stdout: |+"
cat ${stdoutFile} | sed 's/^/    /'
echo "stderr: |+"
cat ${stderrFile} | sed 's/^/    /'


echo "dependencies:"
if [ "${errorCode}" -eq 0 ] && [ -n "${dependencyFile}" ]; then
    parseDepFile ${dependencyFile}
fi

echo "cached: false"
echo "compilable: true"
echo "output_file: ${outputFile}"

if [ $errorCode -ne "0" ]; then
    exit -1
fi
exit 0
