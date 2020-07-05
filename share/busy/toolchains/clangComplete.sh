#!/bin/bash

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
  - name: "clang complete"
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

outFile="compile_commands.json"


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

	echo "clean: true"
	echo "max_jobs: 1"

	cat > ${outFile} <<-END
		[
	END
	exit 0
elif [ "$1" == "end" ]; then
	cat >> ${outFile} <<-END
		{}
		]
	END
	exit 0
elif [ "$1" == "compile" ]; then
	shift; inputFile="$1"
	shift; outputFile="$1"
	shift

	parse "-ilocal  projectIncludes" \
	      "-isystem systemIncludes" \
	      "--" "$@"

	projectIncludes=$(implode " -I " "${projectIncludes[@]}")
	systemIncludes=$(implode " -isystem " "${systemIncludes[@]}")

	filetype="$(echo "${inputFile}" | rev | cut -d "." -f 1 | rev)";

	directory=$(realpath --relative-to / ..)
	if [[ "${filetype}" =~ ^(cpp|cc)$ ]]; then
		call="${CXX} -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c $inputFile -o $outputFile $projectIncludes $systemIncludes"
	elif [ "${filetype}" = "c" ]; then
		call="${C} -O0 -std=c18 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c $inputFile -o $outputFile $projectIncludes $systemIncludes"
	else
		exit 0
	fi

cat >> ${outFile} <<-END
	{ "directory": "${directory}",
	  "command": "${call}",
	  "file": "${inputFile}"},
END

elif [ "$1" == "link" ]; then
	exit 0
else
	exit -1
fi

exit 0
