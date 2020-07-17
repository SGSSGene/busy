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


if [ "$1" == "info" ]; then
shift

cat <<-END
toolchains:
  - name: "clang complete"
    version: "0.0.0"
    detail: "0.0.0"
    which:
    options:
      default: []
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

	echo "rebuild: true"
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

	parse "--ilocal  projectIncludes" \
	      "--isystem systemIncludes" \
	      "--" "$@"

	projectIncludes=$(implode " -I " "${projectIncludes[@]}")
	systemIncludes=$(implode " -isystem " "${systemIncludes[@]}")

	filetype="$(echo "${inputFile}" | rev | cut -d "." -f 1 | rev)";

	directory=/$(realpath --relative-to / ..)
	if [[ "${filetype}" =~ ^(cpp|cc)$ ]]; then
		call="${CXX} -O0 -std=c++20 -c $inputFile -o $outputFile $projectIncludes $systemIncludes"
	elif [ "${filetype}" = "c" ]; then
		call="${C} -O0 -std=c18 -c $inputFile -o $outputFile $projectIncludes $systemIncludes"
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
