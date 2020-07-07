#!/bin/bash

# $ <$0> info
# $ <$0> compile input.cpp output.o -ilocal <includes>... -isystem <system includes>...
# $ <$0> link static_library output.a -i obj1.o obj2.o lib2.a -l pthread armadillo
# $ <$0> link executable output.exe -i obj1.o obj2.o lib2.a -l pthread armadillo

#
# Return values:
# 0 on success
# -1 error

source "${0%/*}"/helper_utils.sh

CXX=/usr/bin/g++
C=/usr/bin/gcc
LD=/usr/bin/ld
AR=/usr/bin/ar

version_detailed="$(${C} --version | head -n 1)"
version=$(echo ${version_detailed} | perl -ne "s/\([^\)]*\)/()/g;print" | cut -d " " -f 3)

version_major=$(echo ${version} | cut -d "." -f 1)
version_minor=$(echo ${version} | cut -d "." -f 2)
version_patch=$(echo ${version} | cut -d "." -f 3)


# check if version numbers match
if [ "${version_major}" != "10" ] || [ "${version_minor}" -lt "1" ]; then
	exit -1
fi


parse "-options    options"\
      "--" "$@"

process_id=$BASHPID

# check if ccache is available
CCACHE=0
if [[ " ${options[@]} " =~ " ccache " ]]; then
	which ccache > /dev/null 2>&1
	if [ "$?" != "0" ]; then
		exit -1
	fi

	CCACHE=1
	export CCACHE_LOGFILE="log/ccache${process_id}.log"
	CXX="ccache ${CXX}"
	C="ccache ${C}"
	LD="ccache ${LD}"
	AR="ccache ${AR}"
fi

if [ "$1" == "info" ]; then
shift

cat <<-END
toolchains:
  - name: "gcc 10.1"
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
      profile: []
      analyze: []
      ccache: []
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
	if [ ! -e "log" ]; then
		mkdir log
	fi
	exit 0
elif [ "$1" == "end" ]; then
	rmdir log
	exit 0
elif [ "$1" == "compile" ]; then
	shift; inputFile="$1"
	shift; outputFile="$1"
	shift

	dependencyFile=$(echo "${outputFile}" | rev | cut -b 3- | rev | xargs -I '{}' echo '{}.d')

	parse "-ilocal  projectIncludes" \
	      "-isystem systemIncludes" \
	      "-options options"\
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
	if [[ " ${options[@]} " =~ " profile " ]]; then
		parameters+=" -fprofile-arcs -ftest-coverage"
	fi
	if [[ " ${options[@]} " =~ " analyze " ]]; then
		CXX="scan-build ${CXX}"
	fi


	projectIncludes+=($(dirname ${projectIncludes[-1]})) #!TODO this line should not be needed
	projectIncludes=$(implode " -I " "${projectIncludes[@]}")
	systemIncludes=$(implode " -isystem " "${systemIncludes[@]}")

	filetype="$(echo "${inputFile}" | rev | cut -d "." -f 1 | rev)";
	if [[ "${filetype}" =~ ^(cpp|cc)$ ]]; then
		call="${CXX} -std=c++20 -fPIC -MD ${parameters} -fdiagnostics-color=always -c ${inputFile} -o ${outputFile} $projectIncludes $systemIncludes"
	elif [ "${filetype}" = "c" ]; then
		call="${C} -std=c18 -fPIC -MD ${parameters} -fdiagnostics-color=always -c ${inputFile} -o ${outputFile} $projectIncludes $systemIncludes"
	else
		exit 0
	fi
elif [ "$1" == "link" ]; then
	shift; target="$1"
	shift; outputFile="$1"
	shift

	parse "-i       inputFiles" \
	      "-il      inputLibraries" \
	      "-l       libraries" \
	      "-options options" \
	      "--" "$@"

	libraries=($(implode " -l" "${libraries[@]}"))


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
	if [[ " ${options[@]} " =~ " profile " ]]; then
		libraries+=("-lgcov")
	fi


	# Header only
	if [ "${#inputFiles[@]}" -eq 0 ]; then
		echo "compilable: false"
		exit 0

	# Executable
	elif [ "${target}" == "executable" ]; then
		call="${CXX} -rdynamic ${parameters} -fdiagnostics-color=always -o ${outputFile} ${inputFiles[@]} ${inputLibraries[@]} ${libraries[@]}"
	# Static library?
	elif [ "${target}" == "static_library" ]; then
		call="${LD} -Ur -o ${outputFile}.o ${inputFiles[@]} && ${AR} rcs ${outputFile} ${outputFile}.o"
	else
		exit -1
	fi
else
	exit -1
fi

stdout="log/stdout${process_id}.log"
stderr="log/stderr${process_id}.log"
dependency="log/dependency${process_id}.log"

mkdir -p $(dirname ${outputFile})
echo > ${stdout}
if [ -n "${set_verbose}" ]; then
	echo $call>>${stdout}
fi
eval $call 1>>${stdout} 2>${stderr}
errorCode=$?

echo "stdout: |+"
cat ${stdout} | sed 's/^/    /'
echo "stderr: |+"
cat ${stderr} | sed 's/^/    /'

echo "dependencies:"
if [ "${errorCode}" -eq 0 ] && [ -n "${dependencyFile}" ]; then
	parseDepFile ${dependencyFile}
fi

is_cached="false"
if [ "${CCACHE}" -eq 1 ]; then
	if [ "$(cat ${CCACHE_LOGFILE} | grep 'Result: cache hit' | wc -l)" -eq 1 ]; then
		is_cached="true"
	fi
	rm ${CCACHE_LOGFILE}
fi
echo "cached: ${is_cached}"
echo "compilable: true"
echo "output_file: ${outputFile}"


rm ${stdout}
rm ${stderr}
if [ -n "${depCall}" ]; then
	rm ${dependency}
fi

if [ $errorCode -ne "0" ]; then
	exit -1
fi
exit 0
