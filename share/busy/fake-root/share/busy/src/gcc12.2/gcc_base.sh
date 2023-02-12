#!/bin/bash

# script must be executed inside the build folder
# $ <$0> info
# $ <$0> init <rootDir>
# $ <$0> finialize <rootDir>
# $ <$0> setup_translation_set <rootDir> <ts_name> -ilocal <includes>... -isystem <system includes>...
# $ <$0> compile <ts_name> input.cpp output.o
# $ <$0> link static_library output.a --input obj1.o obj2.o lib2.a --llibraries pthread armadillo
# $ <$0> link executable output.exe --input obj1.o obj2.o lib2.a --llibraries pthread armadillo

# Return values:
# 0 on success
# -1 error


failure() {
  local lineno=$2
  local fn=$3
  local exitstatus=$4
  local msg=$5
  local lineno_fns=${1% 0}
  if [[ "$lineno_fns" != "0" ]] ; then
    lineno="${lineno} ${lineno_fns}"
  fi
  echo "${BASH_SOURCE[1]}:${fn}[${lineno}] Failed with status ${exitstatus}: $msg"
}
trap 'failure "${BASH_LINENO[*]}" "$LINENO" "${FUNCNAME[*]:-script}" "$?" "$BASH_COMMAND"' ERR

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
    languages: ["c++", "c"]
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
echo "    extraPackages:"
for key in ${!extraPackages[@]}; do
    echo "      - ${key}"
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

    # Accept cmd arguments
    parse "--ilocal  projectIncludes" \
          "--isystem systemIncludes" \
          "--" "$@"

    rm -rf   "environments/${tsName}/includes"
    mkdir -p "environments/${tsName}/includes/local"
    mkdir -p "environments/${tsName}/includes/system"
    mkdir -p "environments/${tsName}/src"
    mkdir -p "environments/${tsName}/obj"


    # Link 'src' into environments
    ln -fs "$(realpath "${rootDir}/src/${tsName}" --relative-to "environments/${tsName}/src")" -T "environments/${tsName}/src/${tsName}"

    # Link project includes into environments
    for f in "${projectIncludes[@]}"; do
        target="environments/${tsName}/includes/local/$(basename "${f}")"
        ln -s "$(realpath "${rootDir}/${f}" --relative-to "$(dirname ${target})")" -T "${target}"
    done

    # Link system includes into environments
    for f in "${systemIncludes[@]}"; do
        i=0
        p1=$(echo ${f} | cut -d ':' -f 1)
        p2=$(echo ${f} | cut -d ':' -f 2)
        target="environments/${tsName}/includes/system/$i"
        while [ -e ${target} ]; do
            i=$(expr $i + 1)
            target="environments/${tsName}/includes/system/$i"
        done
        if [ ! -z "${p2}" ]; then
            target=${target}/${p2}
        fi
        mkdir -p "$(dirname ${target})"
        if [ "${p1:0:1}" != "/" ]; then
            p1="$(realpath "${rootDir}/${p1}" --relative-to "$(dirname ${target})")"
        fi
        ln -s "${p1}" -T "${target}"


    done

    exit 0
elif [ "$1" == "compile" ]; then
    shift; tsName="$1"
    shift; inputFile="$1"
    shift

    shift; outputFile="$1"


    objPath="environments/${tsName}/obj"

    mkdir -p "$(dirname "${objPath}/${inputFile}")"

    objectFile="${objPath}/${inputFile}.o"

    dependencyFile="${objPath}/${inputFile}.d"
    stdoutFile="${objPath}/${inputFile}.stdout"
    stderrFile="${objPath}/${inputFile}.stderr"
    export CCACHE_LOGFILE="${objPath}/${inputFile}.ccache"

    outputFiles+=("${objectFile}" "${dependencyFile}" "${stdoutFile}" "${stderrFile}")
    if [ "${CCACHE}" -eq 1 ]; then
        outputFiles+=(${CCACHE_LOGFILE})
    fi

    parse "--ilocal  projectIncludes" \
          "--isystem systemIncludes" \
          "--" "$@"

    parameters=" -MD "
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

    projectIncludes=$(implode " -iquote " "${projectIncludes[@]}")
    systemIncludes=$(implode " -isystem " "${systemIncludes[@]}")

    diagnostic="-fdiagnostics-color=always -fdiagnostics-show-template-tree -fdiagnostics-format=text"

    # remove all stdlibs
    parameters="${parameters} -nostdinc -nostdinc++"

    inputFile="environments/${tsName}/src/${tsName}/${inputFile}"
    filetype="$(echo "${inputFile}" | rev | cut -d "." -f 1 | rev)";
    if [[ "${filetype}" =~ ^(cpp|cc)$ ]]; then
        call="${CXX} ${CXX_STD} ${parameters} ${diagnostic} -c ${inputFile} -o ${objectFile} $projectIncludes $systemIncludes"
    elif [ "${filetype}" = "c" ]; then
        call="${C} ${C_STD} ${parameters} ${diagnostic} -c ${inputFile} -o ${objectFile} $projectIncludes $systemIncludes"
    else
        echo "stdout:"
        echo "stderr:"
        echo "dependencies: []"
        echo "success: true"
        echo "cached: false"
        echo "compilable: false"
        echo "output_files: []"
        exit 0
    fi
elif [ "$1" == "link" ]; then
    shift; tsName="$1"
    shift; target="$1"
    shift

    parse "--input           inputFiles" \
          "--llibraries      localLibraries" \
          "--syslibrarypaths sysLibraryPaths" \
          "--syslibraries    sysLibraries" \
          "--" "$@"

    outputFile="bin/${tsName}"
    stdoutFile="environments/${tsName}/obj/${tsName}.stdout"
    stderrFile="environments/${tsName}/obj/${tsName}.stderr"
    export CCACHE_LOGFILE="environments/${tsName}/obj/${tsName}.ccache"

    parameters=""
    for key in ${!profile_link_param[@]}; do
        if [[ "${options[@]} " =~ " ${key} " ]]; then
            parameters+="${profile_link_param[$key]}"
        fi
    done

    inputFilesTmp=()
    for i in "${!inputFiles[@]}"; do
        filetype="$(echo "${inputFiles[i]}" | rev | cut -d "." -f 1 | rev)"
        if [[ "${filetype}" =~ ^(cpp|cc)$ ]]; then
            inputFilesTmp+=("environments/${tsName}/obj/${inputFiles[i]}.o")
        fi
    done
    inputFiles=("${inputFilesTmp[@]}")

    # Header only
    if [ "${#inputFiles[@]}" -eq 0 ]; then
        echo "compilable: false"
        echo "success: true"
        echo "cached: false"
        echo "output_files: []"
        exit 0
    fi

    localLibrariesAsStr=""
    for i in "${localLibraries[@]}"; do
        if [ -e "lib/${i}.a" ]; then
            localLibrariesAsStr+=" lib/${i}.a"
        fi
    done

    sysLibraryPaths=($(implode " -L" "${sysLibraryPaths[@]}"))
    sysLibraries=($(implode " -l" "${sysLibraries[@]}"))

    # Executable
    if [ "${target}" == "executable" ]; then
        call="${CXX} -rdynamic ${parameters} -fdiagnostics-color=always -o ${outputFile} ${inputFiles[@]} ${localLibrariesAsStr} ${sysLibraryPaths[@]} ${sysLibraries[@]}"
    # Shared library
    elif [ "${target}" == "shared_library" ]; then
        call="${CXX} -rdynamic -shared ${parameters} -fdiagnostics-color=always -o ${outputFile} ${inputFiles[@]} ${localLibrariesAsStr} ${sysLibraryPaths[@]} ${sysLibraries[@]}"
    # Static library
    elif [ "${target}" == "static_library" ]; then
        outputFile="lib/${tsName}.a"
        objectFile="environments/${tsName}/obj/${tsName}.o"
        mkdir -p $(dirname ${objectFile})
        outputFiles+=("${objectFile}")
        call="${LD} -Ur -o ${objectFile} ${inputFiles[@]} && ${AR} rcs ${outputFile} ${objectFile}"
    else
        echo "unknown target ${target}"
        exit -1
    fi
    outputFiles+=("${outputFile}" "${stdoutFile}" "${stderrFile}")
    if [ "${CCACHE}" -eq 1 ]; then
        outputFiles+=(${CCACHE_LOGFILE})
    fi
else
    exit -1
fi

if [ "${CCACHE}" -eq 1 ]; then
    rm -f ${CCACHE_LOGFILE}
fi

mkdir -p $(dirname ${outputFile})
: > ${stdoutFile}
if [ -n "${set_verbose-}" ]; then
    echo $call>>${stdoutFile}
fi

errorCode=0
eval $call 1>>${stdoutFile} 2>${stderrFile} || errorCode=$?

echo "call: \"${call}\""
echo "stdout: |+"
cat ${stdoutFile} | sed 's/^/    /'
echo "stderr: |+"
cat ${stderrFile} | sed 's/^/    /'


echo "dependencies:"
if [ "${errorCode}" -eq 0 ] && [ -n "${dependencyFile-}" ]; then
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
if [ "${errorCode}" -eq 0 ]; then
   echo "success: true"
else
   echo "success: false"
fi
echo "output_files:"
for f in "${outputFiles[@]}"; do
    echo "  - ${f}"
done
if [ $errorCode -ne "0" ]; then
    exit -1
fi
exit 0
