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
LD=ld

if [ "$1" == "info" ]; then
shift

cat <<-END
toolchains:
  - name: "cmake"
    version: 3.17
    detail: "cmake --version"
    which:
      - "cmake"
    options:
END
exit 0
fi

outFile="CMakeLists.txt"
cacheFile="tmp_cache.txt"

if [ "$1" == "begin" ]; then
	rootDir="$2"
	relPath="$(pwd)"
	cat > ${outFile} <<-END
		cmake_minimum_required(VERSION 3.1.0)
		project("unknown")
		set(CMAKE_CXX_STANDARD 20)
		set(CMAKE_CXX_STANDARD_REQUIRED ON)
		set(CMAKE_CXX_EXTENSIONS OFF)
	END
	cat > ${cacheFile} <<-END
	END
	echo "clean: true"
	echo "max_jobs: 1"
	exit 0
elif [ "$1" == "end" ]; then
	rm ${cacheFile}
	exit 0
elif [ "$1" == "compile" ]; then
	shift; inputFile="$1"
	shift; outputFile="$1"
	shift

	parse "-ilocal  projectIncludes" \
	      "-isystem systemIncludes" \
	      "--" "$@"

	projectIncludes+=($(dirname ${projectIncludes[-1]})) #!TODO this line should not be needed
	projectIncludes=$(implode " " "${projectIncludes[@]}")
	systemIncludes=$(implode " " "${systemIncludes[@]}")


	filetype="$(echo "${inputFile}" | rev | cut -d "." -f 1 | rev)";
	if [[ "${filetype}" =~ ^(cpp|cc|c)$ ]]; then
		echo -c ${inputFile} -o ${outputFile} ${projectIncludes} ${systemIncludes} >> ${cacheFile}
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
	      "--" "$@"

	targetType=
	# Header only
	if [ "${#inputFiles[@]}" -eq 0 ]; then
		exit 0
	# Executable
	elif [ "${target}" == "executable" ]; then
		targetType="add_executable"
	# Static library?
	elif [ "${target}" == "static_library" ]; then
		targetType="add_library"
	else
		exit -1
	fi

	name=$(basename ${outputFile})
	(
		echo "${targetType}(${name}"
		for f in "${inputFiles[@]}"; do
			file=$(cat ${cacheFile} | grep -- "-o ${f}" | cut -d " " -f 2)
			echo "  ${file}"
		done
		echo ")"
		echo "target_include_directories(${name} PUBLIC"
		for i in $(cat ${cacheFile} | grep -- "-o ${inputFiles[0]}" | cut -d " " -f 5-); do
			echo "  $i"
		done
		echo ")"
		if [ "${target}" == "executable" ]; then
			echo "target_link_libraries (${name} LINK_PUBLIC"
			for l in "${inputLibraries[@]}"; do
				echo "  $(basename $l)"
			done
			for l in "${libraries[@]}"; do
				echo "  ${l}"
			done
			echo ")";
	fi

	) >> ${outFile}



else
	exit -1
fi
echo "compilable: true"
exit 0
