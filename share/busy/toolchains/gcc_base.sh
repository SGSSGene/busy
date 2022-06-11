#!/bin/bash

# script must be executed inside the build folder
# $ <$0> info
# $ <$0> init <rootDir>
# $ <$0> finialize <rootDir>
# $ <$0> setup_translation_set <rootDir> <ts_name> -ilocal <includes>... -isystem <system includes>...
# $ <$0> compile <ts_name> input.cpp output.o
# $ <$0> link static_library output.a -i obj1.o obj2.o lib2.a -l pthread armadillo
# $ <$0> link executable output.exe -i obj1.o obj2.o lib2.a -l pthread armadillo

# Return values:
# 0 on success
# -1 error

if [ -z "${BUSY_TOOLCHAIN_CALL}" ]; then
    exit -1
fi

source "${0%/*}"/helper_utils.sh
BUILD_SCRIPTS="${BUILD_SCRIPTS} "${0%/*}"/gcc_base.sh "${0%/*}"/helper_utils.sh"


version_detailed="$(${C} --version | head -n 1)"
version=$(echo ${version_detailed} | perl -ne "s/\([^\)]*\)/()/g;print" | cut -d " " -f 3)

version_major=$(echo ${version} | cut -d "." -f 1)
version_minor=$(echo ${version} | cut -d "." -f 2)
version_patch=$(echo ${version} | cut -d "." -f 3)

# check if version numbers match
if [ "${version_major}" != "${expected_major}" ] || [ "${version_minor}" != "${expected_minor}" ]; then
    exit -1
fi

parse "--options    options"\
      "--verbose    verbose"\
      "--" "$@"

# check if ccache is available
CCACHE=0
if [[ " ${options[@]} " =~ " ccache " ]]; then
    which ccache > /dev/null 2>&1
    if [ "$?" != "0" ]; then
        exit -1
    fi

    CCACHE=1
    CXX="ccache ${CXX}"
    C="ccache ${C}"
    LD="ccache ${LD}"
    AR="ccache ${AR}"
fi

if [ "$1" == "info" ]; then
shift

cat <<-END
toolchains:
  - name: "${NAME}"
    version: "${version}"
    detail: "${version_detailed}"
    which:
      - "${CXX}"
      - "${C}"
      - "${AR}"
      - "${LD}"
END
echo "    options:"
for key in ${!profiles[@]}; do
    deps=(${profiles[$key]})
    echo -n "      $key: ["
    deps=$(printf ", %s" "${deps[@]}")
    deps=${deps:2}
    echo $deps]
done
exit 0
fi


outputFiles=()
if [ "$1" == "init" ]; then
    rootDir="$2"
    if [ ! -e "external" ] && [ -e ${rootDir}/external ]; then
        ln -s ${rootDir}/external external
    fi
    if [ ! -e "src" ] && [ -e ${rootDir}/src ]; then
        ln -s ${rootDir}/src src
    fi

    hash="$(echo "${@}" | cat - ${BUILD_SCRIPTS} ${CXX} ${C} ${LD} ${AR} | shasum)"
    echo "hash: ${hash}";
    exit 0
elif [ "$1" == "finalize" ]; then
    exit 0
elif [ "$1" == "setup_translation_set" ] ; then
    shift; rootDir="$1"
    shift; tsName="$1"
    shift;

    rm -rf environments/${tsName}/includes
    mkdir -p environments/${tsName}/includes/local
    mkdir -p environments/${tsName}/includes/system

    parse "--ilocal  projectIncludes" \
          "--isystem systemIncludes" \
          "--verbose verbose" \
          "--" "$@"
    for f in "${projectIncludes[@]}"; do
        target="environments/${tsName}/includes/local/$(basename "${f}")"
        ln -s "$(realpath "${rootDir}/${f}" --relative-to "$(dirname ${target})")" -T "${target}"
    done
    for f in "${systemIncludes[@]}"; do
        i=0
        p1=$(echo ${f} | cut -d ':' -f 1)
        p2=$(echo ${f} | cut -d ':' -f 2)
        target="environments/${tsName}/includes/system/$i"
        while [ -e ${target} ]; do
            i=$(expr $i + 1)
            target="environments/${tsName}/includes/system/$i"
        done
        target=${target}/${p2}
        mkdir -p "$(dirname ${target})"
        if [ "${p1:0:1}" = "/" ]; then
            ln -s "${p1}" -T "${target}"
        else
            echo realpath "${rootDir}/${p1}" --relative-to "$(dirname ${target})"
            echo ln -s "$(realpath "${rootDir}/${p1}" --relative-to "$(dirname ${target})")" -T "${target}"
            ln -s "$(realpath "${rootDir}/${p1}" --relative-to "$(dirname ${target})")" -T "${target}"
        fi
    done

    exit 0
elif [ "$1" == "compile" ]; then
    shift; tsName="$1"
    shift; inputFile="$1"
    shift; outputFile="$1"
    shift

    dependencyFile=$(echo "${outputFile}" | rev | cut -b 3- | rev | xargs -I '{}' echo '{}.d')
    stdoutFile="tmp/${outputFile}.stdout"
    stderrFile="tmp/${outputFile}.stderr"
    export CCACHE_LOGFILE="tmp/${outputFile}.ccache"
    mkdir -p $(dirname ${stdoutFile})

    outputFiles+=("${outputFile}" "${dependencyFile}" "${stdoutFile}" "${stderrFile}")
    if [ "${CCACHE}" -eq 1 ]; then
        outputFiles+=(${CCACHE_LOGFILE})
    fi

    parse "--ilocal  projectIncludes" \
          "--isystem systemIncludes" \
          "--verbose verbose" \
          "--" "$@"

    parameters=""
    for key in ${!profile_compile_param[@]}; do
        if [[ "${options[@]} " =~ " ${key} " ]]; then
            parameters+="${profile_compile_param[$key]}"
        fi
    done

    i=0
    target="environments/${tsName}/includes/system/$i"
    while [ -d ${target} ]; do
        systemIncludes+=("${target}")
        i=$(expr $i + 1)
        target="environments/${tsName}/includes/system/$i"
    done

    projectIncludes+=($(dirname ${projectIncludes[-1]})) #!TODO this line should not be needed
    projectIncludes=$(implode " -iquote " "${projectIncludes[@]}")
    systemIncludes=$(implode " -isystem " "${systemIncludes[@]}")

    diagnostic="-fdiagnostics-color=always -fdiagnostics-show-template-tree -fdiagnostics-format=text"

    parameters="${parameters} -nostdinc -nostdinc++
        -isystem /usr/include/c++/12.1.0
        -isystem /usr/include/c++/12.1.0/x86_64-pc-linux-gnu/
        -isystem /usr/include/c++/12.1.0/backward
        -isystem /usr/lib/gcc/x86_64-pc-linux-gnu/12.1.0/include
        -isystem /usr/lib/gcc/x86_64-pc-linux-gnu/12.1.0/include-fixed
        -isystem \"${0%/*}\"/gcc12.1-includes"


    filetype="$(echo "${inputFile}" | rev | cut -d "." -f 1 | rev)";
    if [[ "${filetype}" =~ ^(cpp|cc)$ ]]; then
        call="${CXX} ${CXX_STD} ${parameters} ${diagnostic} -c ${inputFile} -o ${outputFile} $projectIncludes $systemIncludes"
    elif [ "${filetype}" = "c" ]; then
        call="${C} ${C_STD} ${parameters} ${diagnostic} -c ${inputFile} -o ${outputFile} $projectIncludes $systemIncludes"
    else
        exit 0
    fi
elif [ "$1" == "link" ]; then
    shift; target="$1"
    shift; outputFile="$1"
    shift

    stdoutFile="tmp/${outputFile}.stdout"
    stderrFile="tmp/${outputFile}.stderr"
    export CCACHE_LOGFILE="tmp/${outputFile}.ccache"
    mkdir -p $(dirname ${stdoutFile})

    outputFiles+=("${outputFile}" "${stdoutFile}" "${stderrFile}")
    if [ "${CCACHE}" -eq 1 ]; then
        outputFiles+=(${CCACHE_LOGFILE})
    fi

    parse "--input         inputFiles" \
          "--llibraries    localLibraries" \
          "--syslibraries  sysLibraries" \
          "--verbose       verbose" \
          "--" "$@"

    parameters=""
    for key in ${!profile_link_param[@]}; do
        if [[ "${options[@]} " =~ " ${key} " ]]; then
            parameters+="${profile_link_param[$key]}"
        fi
    done
    for key in ${!profile_link_libraries[@]}; do
        if [[ "${options[@]} " =~ " ${key} " ]]; then
            sysLibraries+=("${profile_link_libraries[$key]}")
        fi
    done

    sysLibraries=($(implode " -l" "${sysLibraries[@]}"))

    # Header only
    if [ "${#inputFiles[@]}" -eq 0 ]; then
        echo "compilable: false"
        exit 0

    # Executable
    elif [ "${target}" == "executable" ]; then
        call="${CXX} -rdynamic ${parameters} -fdiagnostics-color=always -o ${outputFile} ${inputFiles[@]} ${localLibraries[@]} ${sysLibraries[@]}"
    # Shared library
    elif [ "${target}" == "shared_library" ]; then
        call="${CXX} -rdynamic -shared ${parameters} -fdiagnostics-color=always -o ${outputFile} ${inputFiles[@]} ${localLibraries[@]} ${sysLibraries[@]}"
    # Static library
    elif [ "${target}" == "static_library" ]; then
        objectFile="tmp/lib/${outputFile}.o"
        mkdir -p $(dirname ${objectFile})
        outputFiles+=("${objectFile}")
        call="${LD} -Ur -o ${objectFile} ${inputFiles[@]} && ${AR} rcs ${outputFile} ${objectFile}"

    else
        exit -1
    fi
else
    exit -1
fi

if [ "${CCACHE}" -eq 1 ]; then
    rm -f ${CCACHE_LOGFILE}
fi

mkdir -p $(dirname ${outputFile})
: > ${stdoutFile}
if [ -n "${set_verbose}" ]; then
    echo $call>>${stdoutFile}
fi
eval $call 1>>${stdoutFile} 2>${stderrFile}
errorCode=$?

echo "stdout: |+"
echo $REAL_CALL | sed 's/^/    /'
echo $call | sed 's/^/    /'
cat ${stdoutFile} | sed 's/^/    /'
echo "stderr: |+"
cat ${stderrFile} | sed 's/^/    /'


echo "dependencies:"
if [ "${errorCode}" -eq 0 ] && [ -n "${dependencyFile}" ]; then
    parseDepFile ${dependencyFile}
fi

is_cached="false"
if [ "${CCACHE}" -eq 1 ] && [ -f "${CCACHE_LOGFILE}" ]; then
    if [ "$(cat ${CCACHE_LOGFILE} | grep 'Result: direct_cache_hit' | wc -l)" -eq 1 ]; then
        is_cached="true"
    fi
    if [ "$(cat ${CCACHE_LOGFILE} | grep 'Result: preprocessed_cache_hit' | wc -l)" -eq 1 ]; then
        is_cached="true"
    fi

fi
echo "cached: ${is_cached}"
echo "compilable: true"
echo "output_files:"
for f in "${outputFiles[@]}"; do
    echo "  - ${f}"
done
if [ $errorCode -ne "0" ]; then
    exit -1
fi
exit 0
