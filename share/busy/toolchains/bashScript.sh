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

if [ "$1" == "info" ]; then
shift

cat <<-END
toolchains:
  - name: "bash script"
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

outFile="bashScript.sh"


if [ $# -lt 5 ]; then
	if [ "$1" == "begin" ]; then
		cat > ${outFile} <<-END
			#!/bin/bash
			if [ ! -e "external" ]; then
			    ln -s ../external external
			fi
			if [ ! -e "src" ]; then
			    ln -s ../src src
			fi
		END
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

	projectIncludes+=($(dirname ${projectIncludes[-1]})) #!TODO this line should not be needed
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

echo "mkdir -p $(dirname ${outputFile})" >> ${outFile}
echo "${call}" >> ${outFile}
exit 0
