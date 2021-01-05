#!/bin/bash

# $ <$0> info
# $ <$0> compile input.cpp output.o -ilocal <includes>... -isystem <system includes>...
# $ <$0> link static_library output.a -i obj1.o obj2.o lib2.a -l pthread armadillo
# $ <$0> link executable output.exe -i obj1.o obj2.o lib2.a -l pthread armadillo

# Return values:
# 0 on success
# -1 error

source "${0%/*}"/helper_utils.sh

CXX=/usr/bin/clang++
C=/usr/bin/clang
LD=/usr/bin/ld
AR=/usr/bin/ar

version_detailed="$(${C} --version | head -n 1)"
version=$(echo ${version_detailed} | perl -ne "s/\([^\)]*\)/()/g;print" | cut -d " " -f 3)

version_major=$(echo ${version} | cut -d "." -f 1)
version_minor=$(echo ${version} | cut -d "." -f 2)
version_patch=$(echo ${version} | cut -d "." -f 3)

# check if version numbers match
if [ "${version_major}" != "11" ] || [ "${version_minor}" -lt "0" ]; then
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
  - name: "clang 11.0"
    version: "${version}"
    detail: "${version_detailed}"
    which:
      - "${CXX}"
      - "${C}"
      - "${AR}"
      - "${LD}"
    options:
      default: [debug, ccache, no-default]
      release: [no-debug, no-release_with_symbols]
      release_with_symbols: [no-release, no-debug]
      debug: [no-release, no-release_with_symbols]
      ccache: []
      strict: []
END
exit 0
fi


outputFiles=()
if [ "$1" == "begin" ]; then
	rootDir="$2"
	if [ ! -e "external" ] && [ -e ${rootDir}/external ]; then
		ln -s ${rootDir}/external external
	fi
	if [ ! -e "src" ] && [ -e ${rootDir}/src ]; then
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
	if [[ " ${options[@]} " =~ " release " ]]; then
		parameters+=" -O2";
	fi
	if [[ " ${options[@]} " =~ " release_with_symbols " ]]; then
		parameters+=" -O2 -ggdb";
	fi
	if [[ " ${options[@]} " =~ " debug " ]]; then
		parameters+=" -O0 -ggdb";
	fi
	if [[ " ${options[@]} " =~ " strict " ]]; then
		parameters+=" -Wall -Wextra -Wpedantic"
	fi

	projectIncludes+=($(dirname ${projectIncludes[-1]})) #!TODO this line should not be needed
	projectIncludes=$(implode " -I " "${projectIncludes[@]}")
	systemIncludes=$(implode " -isystem " "${systemIncludes[@]}")

	diagnostic="-fdiagnostics-color=always -fdiagnostics-show-template-tree"

	filetype="$(echo "${inputFile}" | rev | cut -d "." -f 1 | rev)";
	if [[ "${filetype}" =~ ^(cpp|cc)$ ]]; then
		call="${CXX} -std=c++20 -fPIC -MMD ${parameters} ${diagnostic} -c ${inputFile} -o ${outputFile} $projectIncludes $systemIncludes"
	elif [ "${filetype}" = "c" ]; then
		call="${C} -std=c18 -fPIC -MMD ${parameters} ${diagnostic} -c ${inputFile} -o ${outputFile} $projectIncludes $systemIncludes"
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
	if [[ " ${options[@]} " =~ " release " ]]; then
		parameters+="";
	fi
	if [[ " ${options[@]} " =~ " release_with_symbols " ]]; then
		parameters+=" -g3 -ggdb";
	fi
	if [[ " ${options[@]} " =~ " debug " ]]; then
		parameters+=" -g3 -ggdb";
	fi

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

mkdir -p $(dirname ${outputFile})
: > ${stdoutFile}
if [ -n "${set_verbose}" ]; then
	echo $call>>${stdoutFile}
fi
eval $call 1>>${stdoutFile} 2>${stderrFile}
errorCode=$?

echo "stdout: |+"
cat ${stdoutFile} | sed 's/^/    /'
echo "stderr: |+"
cat ${stderrFile} | sed 's/^/    /'

echo "dependencies:"
if [ "${errorCode}" -eq 0 ] && [ -n "${dependencyFile}" ]; then
	parseDepFile ${dependencyFile}
fi

is_cached="false"
if [ "${CCACHE}" -eq 1 ]; then
	if [ "$(cat ${CCACHE_LOGFILE} | grep 'Result: cache hit' | wc -l)" -eq 1 ]; then
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
