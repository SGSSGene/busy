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

CXX=/usr/bin/g++
C=/usr/bin/gcc
AR=/usr/bin/ar

version=$(${C} --version | head -n 1 | perl -ne "s/\([^\)]*\)/()/g;print" | cut -d " " -f 3)

version_major=$(echo ${version} | cut -d "." -f 1)
version_minor=$(echo ${version} | cut -d "." -f 2)
version_patch=$(echo ${version} | cut -d "." -f 3)


# check if version numbers match
if [ "${version_major}" != "10" ] || [ "${version_minor}" != "1" ]; then
	exit -1
fi

# todo check if platform matches

# check if ccache is available

parse "-options options"\
      "--" "$@"
CCACHE=0
if [[ " ${options[@]} " =~ " ccache " ]]; then
	which ccache > /dev/null 2>&1
	if [ "$?" != "0" ]; then
		exit -1
	fi

	CCACHE=1
	export CCACHE_LOGFILE=ccache.log
	CXX="ccache ${CXX}"
	C="ccache ${C}"
	AR="ccache ${AR}"
fi

if [ "$1" == "info" ]; then
shift

cat <<-END
toolchains:
  - name: "gcc 10.1"
    version: ${version}
    detail: "$(${CXX} --version | head -1)"
    which:
      - "${CXX}"
      - "${C}"
      - "${AR}"
    options:
      release: [no-debug, no-release_with_symbols]
      release_with_symbols: [no-release, no-debug]
      debug: [no-release, no-release_with_symbols]
      ccache: []
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

	dependencyFile=$(echo "${outputFile}" | rev | cut -b 3- | rev | xargs -I '{}' echo '{}.d')

	parse "-ilocal  projectIncludes" \
	      "-isystem systemIncludes" \
	      "-options options"\
	      "--" "$@"

	parameters=""
	if [[ " ${options[@]} " =~ " release " ]]; then
		parameters+=" -O2";
	elif [[ " ${options[@]} " =~ " release_with_symbols " ]]; then
		parameters+=" -O2 -ggdb";
	elif [[ " ${options[@]} " =~ " debug " ]]; then
		parameters+=" -O0 -ggdb";
	fi

	projectIncludes+=($(dirname ${projectIncludes[-1]})) #!TODO this line should not be needed
	projectIncludes=$(implode " -I " "${projectIncludes[@]}")
	systemIncludes=$(implode " -isystem " "${systemIncludes[@]}")

	filetype="$(echo "${inputFile}" | rev | cut -d "." -f 1 | rev)";
	if [ "${filetype}" = "cpp" ]; then
		call="${CXX} -O0 -std=c++20 -fPIC -MD ${parameters} -fdiagnostics-color=always -c ${inputFile} -o ${outputFile} $projectIncludes $systemIncludes"
		depCall='cat "${dependencyFile}" | xargs -n1 echo | sed "/^$/d" | tail -n +2'

	elif [ "${filetype}" = "c" ]; then
		call="${C} -O0 -std=c18 -fPIC -MD ${parameters} -fdiagnostics-color=always -c ${inputFile} -o ${outputFile} $projectIncludes $systemIncludes"
		depCall='cat "${dependencyFile}" | xargs -n1 echo | sed "/^$/d" | tail -n +2'
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
	      "-options options" \
	      "--" "$@"

	libraries=($(implode " -l" "${libraries[@]}"))

	# Header only
	if [ "${#inputFiles[@]}" -eq 0 ]; then
		exit 1

	# Executable
	elif [ "${target}" == "executable" ]; then
		call="${CXX} -rdynamic -g3 -ggdb -fdiagnostics-color=always -o ${outputFile} ${inputFiles[@]} ${inputLibraries[@]} ${libraries[@]}"

	# Static library?
	elif [ "${target}" == "static_library" ]; then
		call="${AR} rcs ${outputFile} ${inputFiles[@]}"
	else
		exit -1
	fi
else
	exit -1
fi

mkdir -p $(dirname ${outputFile})
#echo $call
eval $call 1>stdout.log 2>stderr.log
errorCode=$?

#echo $depCall
eval $depCall 1>dependency.log

echo "stdout: |+"
cat stdout.log | sed 's/^/    /'
echo "stderr: |+"
cat stderr.log | sed 's/^/    /'

if [ "${errorCode}" -eq 0 ]; then
	echo "dependencies:"
	cat dependency.log | xargs -n1 echo "  -" | sort
fi

if [ "${CCACHE}" -eq 1 ]; then
	if [ "$(cat ${CCACHE_LOGFILE} | grep 'Result: cache hit' | wc -l)" -eq 1 ]; then
		echo "cached: true"
	else
		echo "cached: false"
	fi
	rm ${CCACHE_LOGFILE}
else
	echo "cached: false"
fi


rm stdout.log
rm stderr.log
rm dependency.log

if [ $errorCode -ne "0" ]; then
	exit -1
fi
exit 0