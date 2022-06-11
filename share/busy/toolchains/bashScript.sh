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
END
exit 0
fi

outFile="bashScript.sh"

if [ "$1" == "init" ]; then
    rootDir="$2"
    relPath="$(pwd)"
cat > ${outFile} <<-END
#!/bin/bash

set -Eeuo pipefail

cd ${relPath}
rootDir="${rootDir}"

if [ ! -e "external" ]; then
    ln -s ${rootDir}/external external
fi
if [ ! -e "src" ]; then
    ln -s ${rootDir}/src src
fi
function setupTranslationSet {
    local tsName="${1}"
    # setup translation set \${tsName}
    rm -rf environments/\${tsName}/includes
    mkdir -p environments/\${tsName}/includes/local
    mkdir -p environments/\${tsName}/includes/system
}
function setupSystemIncludesPaths {
    local tsName="\${1}"; shift
    for f in \${@}; do
        local i=0
        p1=\$(echo \${f} | cut -d ':' -f 1)
        p2=\$(echo \${f} | cut -d ':' -f 2)
        target="environments/\${tsName}/includes/system/\$i"
        while [ -e \${target} ]; do
            i=\$(expr \$i + 1)
            target="environments/\${tsName}/includes/system/\$i"
        done
        target=\${target}/\${p2}
        mkdir -p "\$(dirname \${target})"
        if [ "\${p1:0:1}" = "/" ]; then
            ln -s "\${p1}" -T "\${target}"
        else
            ln -s "\$(realpath "\${rootDir}/\${p1}" --relative-to "\$(dirname \${target})")" -T "\${target}"
        fi
    done
}
function setupSystemIncludes {
    local tsName="\${1}"; shift
    local i=0
    local target="environments/\${tsName}/includes/system/\$i"
    local systemIncludes=""
    while [ -d \${target} ]; do
        systemIncludes="\${systemIncludes} -isystem \"\${target}\""
        i=\$(expr \$i + 1)
        target="environments/\${tsName}/includes/system/\$i"
    done
    systemIncludes=\$(echo \$systemIncludes | xargs -n 1 echo) # split arguments into singular arguments
    echo \${systemIncludes}
}
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
elif [ "$1" == "finalize" ]; then
    exit 0
elif [ "$1" == "setup_translation_set" ] ; then
    shift; rootDir="$1"
    shift; tsName="$1"
    shift;

echo setupTranslationSet "${tsName}" >> ${outFile}

parse "--ilocal  projectIncludes" \
      "--isystem systemIncludes" \
      "--verbose verbose" \
      "--" "$@"


for f in "${projectIncludes[@]}"; do
cat >> ${outFile} <<-END
    target="environments/${tsName}/includes/local/$(basename "${f}")"
    ln -s "\$(realpath "${rootDir}/${f}" --relative-to "\$(dirname \${target}"))" -T "\${target}"
END
done

if [ ${#systemIncludes} -gt 0 ]; then
echo setupSystemIncludesPaths "${rootDir}" "${tsName}" "${systemIncludes[@]}" >> ${outFile}
fi

    exit 0
elif [ "$1" == "compile" ]; then
    shift; tsName="$1"
    shift; inputFile="$1"
    shift; outputFile="$1"
    shift

    parse "--ilocal  projectIncludes" \
          "--isystem systemIncludes" \
          "--verbose verbose" \
          "--" "$@"

    projectIncludes+=($(dirname ${projectIncludes[-1]})) #!TODO this line should not be needed
    projectIncludes=$(implode " -iquote " "${projectIncludes[@]}")
    systemIncludes=$(implode " -isystem " "${systemIncludes[@]}")

    filetype="$(echo "${inputFile}" | rev | cut -d "." -f 1 | rev)";
    if [[ "${filetype}" =~ ^(cpp|cc)$ ]]; then
        call="${CXX} -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c $inputFile -o $outputFile $projectIncludes $systemIncludes \$systemIncludes"
    elif [ "${filetype}" = "c" ]; then
        call="${C} -O0 -std=c18 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c $inputFile -o $outputFile $projectIncludes $systemIncludes \$systemIncludes"
    else
        exit 0
    fi
cat >> ${outFile} <<-END
# compile ${tsName}
    systemIncludes="\$(setupSystemIncludes "${tsName}")"
END

elif [ "$1" == "link" ]; then
    shift; target="$1"
    shift; outputFile="$1"
    shift

    parse "--input        inputFiles" \
          "--llibraries   inputLibraries" \
          "--syslibraries libraries" \
          "--verbose verbose" \
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
cat >> ${outFile} <<-END
# linking ${outputFile} as ${target}
END


else
    exit -1
fi
echo "compilable: true"
lastDir=$(cat ${outFile} | grep "mkdir -p" | tail -n 1 | cut -b 10-)
newDir="$(dirname ${outputFile})"
if [ "${lastDir}" != "${newDir}" ]; then
    echo "    mkdir -p $(dirname ${outputFile})" >> ${outFile}
fi
echo "    ${call}" | sed -e 's/[[:space:]]*$//'>> ${outFile}
exit 0
