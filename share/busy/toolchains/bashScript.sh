#!/bin/bash

# $ <$0> info
# $ <$0> compile input.cpp output.o -ilocal <includes>... -isystem <system includes>...
# $ <$0> link static_library output.a -i obj1.o obj2.o lib2.a -l pthread armadillo
# $ <$0> link executable output.exe -i obj1.o obj2.o lib2.a -l pthread armadillo

# Return values:
# 0 on success
# -1 error

source "${0%/*}"/helper_utils.sh

CXX=g++
C=gcc
AR=ar
LD=ld

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
      - "${LD}"
    options:
      release: [no-debug, no-release_with_symbols]
      release_with_symbols: [no-release, no-debug]
      debug: [no-release, no-release_with_symbols]
      ccache: []
END
exit 0
fi

outFile="bashScript.sh"


if [ "$1" == "begin" ]; then
	rootDir="$2"
	relPath="$(pwd)"
	cat > ${outFile} <<-END
		#!/bin/bash
		cd ${relPath}

		if [ ! -e "external" ]; then
		    ln -s ${rootDir}/external external
		fi
		if [ ! -e "src" ]; then
		    ln -s ${rootDir}/src src
		fi
	END
	if [ ! -e "external" ]; then
	    ln -s ${rootDir}/external external
	fi
	if [ ! -e "src" ]; then
	    ln -s ${rootDir}/src src
	fi
	echo "rebuild: true"
	echo "max_jobs: 1"
	exit 0
elif [ "$1" == "end" ]; then
	exit 0
elif [ "$1" == "compile" ]; then
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
	if [[ "${filetype}" =~ ^(cpp|cc)$ ]]; then
		call="${CXX} -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c $inputFile -o $outputFile $projectIncludes $systemIncludes"
	elif [ "${filetype}" = "c" ]; then
		call="${C} -O0 -std=c18 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c $inputFile -o $outputFile $projectIncludes $systemIncludes"
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

	libraries=($(implode " -l" "${libraries[@]}"))

	# Header only
	if [ "${#inputFiles[@]}" -eq 0 ]; then
		exit 0
	# Executable
	elif [ "${target}" == "executable" ]; then
		call="${CXX} -rdynamic -g3 -ggdb -fdiagnostics-color=always -o $outputFile ${inputFiles[@]} ${inputLibraries[@]} ${libraries[@]}"

	# Static library?
	elif [ "${target}" == "static_library" ]; then
		call="${LD} -Ur -o ${outputFile}.o ${inputFiles[@]} && ${AR} rcs ${outputFile} ${outputFile}.o"
	else
		exit -1
	fi
else
	exit -1
fi
echo "compilable: true"
lastDir=$(cat ${outFile} | grep "mkdir -p" | tail -n 1 | cut -b 10-)
newDir="$(dirname ${outputFile})"
if [ "${lastDir}" != "${newDir}" ]; then
	echo "mkdir -p $(dirname ${outputFile})" >> ${outFile}
fi
echo "${call}" | sed -e 's/[[:space:]]*$//'>> ${outFile}
exit 0
