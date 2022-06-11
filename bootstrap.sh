#!/bin/bash

set -Eeuo pipefail
mkdir -p bootstrap.d
cd bootstrap.d

rootDir=".."

if [ ! -e "external" ]; then
    ln -s ../external external
fi
if [ ! -e "src" ]; then
    ln -s ../src src
fi
function setupTranslationSet {
    local tsName="init"
    # setup translation set ${tsName}
    rm -rf environments/${tsName}/includes
    mkdir -p environments/${tsName}/includes/local
    mkdir -p environments/${tsName}/includes/system
}
function setupSystemIncludesPaths {
    local tsName="${1}"; shift
    for f in ${@}; do
        local i=0
        p1=$(echo ${f} | cut -d ':' -f 1)
        p2=$(echo ${f} | cut -d ':' -f 2)
        target="environments/${tsName}/includes/system/$i"
        while [ -e ${target} ]; do
            i=$(expr $i + 1)
            target="environments/${tsName}/includes/system/$i"
        done
        target=${target}/${p2}
        mkdir -p "$(dirname ${target})"
        if [ "${p1:0:1}" = "/" ]; then
            ln -s "${p1}" -T "${target}"
        else
            ln -s "$(realpath "${rootDir}/${p1}" --relative-to "$(dirname ${target})")" -T "${target}"
        fi
    done
}
function setupSystemIncludes {
    local tsName="${1}"; shift
    local i=0
    local target="environments/${tsName}/includes/system/$i"
    local systemIncludes=""
    while [ -d ${target} ]; do
        systemIncludes="${systemIncludes} -isystem \"${target}\""
        i=$(expr $i + 1)
        target="environments/${tsName}/includes/system/$i"
    done
    systemIncludes=$(echo $systemIncludes | xargs -n 1 echo) # split arguments into singular arguments
    echo ${systemIncludes}
}
setupTranslationSet src/base64
setupTranslationSet src/busy
setupSystemIncludesPaths .. src/busy src/base64:base64 src/fon:fon src/picosha2:picosha2 external/sargparse/src/sargparse:sargparse external/yaml-cpp/src/yaml-cpp:yaml-cpp external/fmt/src/fmt:fmt external/fmt/include/fmt:fmt external/SelfTest/external/Process/src/process:process external/yaml-cpp/src/yaml-cpp:yaml-cpp
setupTranslationSet src/demangle
setupTranslationSet external/sargparse/src/exampleSargparse
setupSystemIncludesPaths .. external/sargparse/src/exampleSargparse external/sargparse/src/sargparse:sargparse
setupTranslationSet external/fmt/src/fmt
setupTranslationSet src/fon
setupSystemIncludesPaths .. src/fon external/yaml-cpp/src/yaml-cpp:yaml-cpp
setupTranslationSet external/gtest/src/gtest
setupTranslationSet src/picosha2
setupTranslationSet external/SelfTest/external/Process/src/process
setupTranslationSet external/sargparse/src/sargparse
setupTranslationSet external/SelfTest/src/selfTest
setupSystemIncludesPaths .. external/SelfTest/src/selfTest external/SelfTest/external/Process/src/process:process
setupTranslationSet src/testBase64
setupSystemIncludesPaths .. src/testBase64 src/base64:base64
setupTranslationSet external/SelfTest/external/Process/src/testProcess
setupSystemIncludesPaths .. external/SelfTest/external/Process/src/testProcess external/SelfTest/external/Process/src/process:process
setupTranslationSet external/SelfTest/src/testSelfTest
setupSystemIncludesPaths .. external/SelfTest/src/testSelfTest external/SelfTest/src/selfTest:selfTest external/SelfTest/external/Process/src/process:process
setupTranslationSet external/ThreadPool/src/testThreadPool
setupSystemIncludesPaths .. external/ThreadPool/src/testThreadPool external/gtest/src/gtest:gtest external/gtest/include/gtest:gtest external/ThreadPool/src/threadPool:threadPool
setupTranslationSet external/yaml-cpp/src/testsYaml
setupSystemIncludesPaths .. external/yaml-cpp/src/testsYaml external/yaml-cpp/src/yaml-cpp:yaml-cpp
setupTranslationSet external/ThreadPool/src/threadPool
setupTranslationSet external/yaml-cpp/src/yaml-cpp
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/busy.cpp -o obj/src/busy/busy.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/FileCache.cpp -o obj/src/busy/FileCache.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/toolchains.cpp -o obj/src/busy/toolchains.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/cache.cpp -o obj/src/busy/cache.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/completion.cpp -o obj/src/busy/completion.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/utils.cpp -o obj/src/busy/utils.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy/cmd
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/cmd/clean.cpp -o obj/src/busy/cmd/clean.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy/cmd
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/cmd/lsToolchains.cpp -o obj/src/busy/cmd/lsToolchains.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy/cmd
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/cmd/version.cpp -o obj/src/busy/cmd/version.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy/cmd
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/cmd/status.cpp -o obj/src/busy/cmd/status.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy/cmd
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/cmd/showPackages.cpp -o obj/src/busy/cmd/showPackages.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy/cmd
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/cmd/compile.cpp -o obj/src/busy/cmd/compile.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy/cmd
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/cmd/showDeps.cpp -o obj/src/busy/cmd/showDeps.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/CompilePipe.cpp -o obj/src/busy/CompilePipe.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/File.cpp -o obj/src/busy/File.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/ConsolePrinter.cpp -o obj/src/busy/ConsolePrinter.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/analyse.cpp -o obj/src/busy/analyse.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/TranslationSet.cpp -o obj/src/busy/TranslationSet.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/busy
    systemIncludes="$(setupSystemIncludes "src/busy")"
    mkdir -p obj/src/busy
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/busy/Package.cpp -o obj/src/busy/Package.o  -iquote src/busy -iquote src  $systemIncludes
# compile src/demangle
    systemIncludes="$(setupSystemIncludes "src/demangle")"
    mkdir -p obj/src/demangle
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/demangle/demangle.cpp -o obj/src/demangle/demangle.o  -iquote src/demangle -iquote src  $systemIncludes
# compile src/base64
    systemIncludes="$(setupSystemIncludes "src/base64")"
    mkdir -p obj/src/base64
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/base64/base64.cpp -o obj/src/base64/base64.o  -iquote src/base64 -iquote src  $systemIncludes
# compile src/testBase64
    systemIncludes="$(setupSystemIncludes "src/testBase64")"
    mkdir -p obj/src/testBase64
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../src/testBase64/main.cpp -o obj/src/testBase64/main.o  -iquote src/testBase64 -iquote src  $systemIncludes
# compile external/sargparse/src/exampleSargparse
    systemIncludes="$(setupSystemIncludes "external/sargparse/src/exampleSargparse")"
    mkdir -p obj/external/sargparse/src/exampleSargparse
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/sargparse/src/exampleSargparse/exampleSargparse.cpp -o obj/external/sargparse/src/exampleSargparse/exampleSargparse.o  -iquote external/sargparse/src/exampleSargparse -iquote external/sargparse/src  $systemIncludes
# compile external/sargparse/src/exampleSargparse
    systemIncludes="$(setupSystemIncludes "external/sargparse/src/exampleSargparse")"
    mkdir -p obj/external/sargparse/src/exampleSargparse
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/sargparse/src/exampleSargparse/exampleSargparse_Add.cpp -o obj/external/sargparse/src/exampleSargparse/exampleSargparse_Add.o  -iquote external/sargparse/src/exampleSargparse -iquote external/sargparse/src  $systemIncludes
# compile external/sargparse/src/exampleSargparse
    systemIncludes="$(setupSystemIncludes "external/sargparse/src/exampleSargparse")"
    mkdir -p obj/external/sargparse/src/exampleSargparse
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/sargparse/src/exampleSargparse/exampleSargparse_NamedAdd.cpp -o obj/external/sargparse/src/exampleSargparse/exampleSargparse_NamedAdd.o  -iquote external/sargparse/src/exampleSargparse -iquote external/sargparse/src  $systemIncludes
# compile external/sargparse/src/exampleSargparse
    systemIncludes="$(setupSystemIncludes "external/sargparse/src/exampleSargparse")"
    mkdir -p obj/external/sargparse/src/exampleSargparse
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/sargparse/src/exampleSargparse/exampleSargparse_Foo.cpp -o obj/external/sargparse/src/exampleSargparse/exampleSargparse_Foo.o  -iquote external/sargparse/src/exampleSargparse -iquote external/sargparse/src  $systemIncludes
# compile external/sargparse/src/sargparse
    systemIncludes="$(setupSystemIncludes "external/sargparse/src/sargparse")"
    mkdir -p obj/external/sargparse/src/sargparse
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/sargparse/src/sargparse/Parameter.cpp -o obj/external/sargparse/src/sargparse/Parameter.o  -iquote external/sargparse/src/sargparse -iquote external/sargparse/src  $systemIncludes
# compile external/sargparse/src/sargparse
    systemIncludes="$(setupSystemIncludes "external/sargparse/src/sargparse")"
    mkdir -p obj/external/sargparse/src/sargparse
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/sargparse/src/sargparse/ArgumentParsing.cpp -o obj/external/sargparse/src/sargparse/ArgumentParsing.o  -iquote external/sargparse/src/sargparse -iquote external/sargparse/src  $systemIncludes
# compile external/sargparse/src/sargparse
    systemIncludes="$(setupSystemIncludes "external/sargparse/src/sargparse")"
    mkdir -p obj/external/sargparse/src/sargparse
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/sargparse/src/sargparse/man.cpp -o obj/external/sargparse/src/sargparse/man.o  -iquote external/sargparse/src/sargparse -iquote external/sargparse/src  $systemIncludes
# compile external/gtest/src/gtest
    systemIncludes="$(setupSystemIncludes "external/gtest/src/gtest")"
    mkdir -p obj/external/gtest/src/gtest/src
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/gtest/src/gtest/src/gtest-port.cc -o obj/external/gtest/src/gtest/src/gtest-port.o  -iquote external/gtest/src/gtest -iquote external/gtest/include/gtest:gtest -iquote external/gtest/include  $systemIncludes
# compile external/gtest/src/gtest
    systemIncludes="$(setupSystemIncludes "external/gtest/src/gtest")"
    mkdir -p obj/external/gtest/src/gtest/src
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/gtest/src/gtest/src/gtest-typed-test.cc -o obj/external/gtest/src/gtest/src/gtest-typed-test.o  -iquote external/gtest/src/gtest -iquote external/gtest/include/gtest:gtest -iquote external/gtest/include  $systemIncludes
# compile external/gtest/src/gtest
    systemIncludes="$(setupSystemIncludes "external/gtest/src/gtest")"
    mkdir -p obj/external/gtest/src/gtest/src
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/gtest/src/gtest/src/gtest_main.cpp -o obj/external/gtest/src/gtest/src/gtest_main.o  -iquote external/gtest/src/gtest -iquote external/gtest/include/gtest:gtest -iquote external/gtest/include  $systemIncludes
# compile external/gtest/src/gtest
    systemIncludes="$(setupSystemIncludes "external/gtest/src/gtest")"
    mkdir -p obj/external/gtest/src/gtest/src
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/gtest/src/gtest/src/gtest-test-part.cc -o obj/external/gtest/src/gtest/src/gtest-test-part.o  -iquote external/gtest/src/gtest -iquote external/gtest/include/gtest:gtest -iquote external/gtest/include  $systemIncludes
# compile external/gtest/src/gtest
    systemIncludes="$(setupSystemIncludes "external/gtest/src/gtest")"
    mkdir -p obj/external/gtest/src/gtest/src
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/gtest/src/gtest/src/gtest-printers.cc -o obj/external/gtest/src/gtest/src/gtest-printers.o  -iquote external/gtest/src/gtest -iquote external/gtest/include/gtest:gtest -iquote external/gtest/include  $systemIncludes
# compile external/gtest/src/gtest
    systemIncludes="$(setupSystemIncludes "external/gtest/src/gtest")"
    mkdir -p obj/external/gtest/src/gtest/src
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/gtest/src/gtest/src/gtest.cc -o obj/external/gtest/src/gtest/src/gtest.o  -iquote external/gtest/src/gtest -iquote external/gtest/include/gtest:gtest -iquote external/gtest/include  $systemIncludes
# compile external/gtest/src/gtest
    systemIncludes="$(setupSystemIncludes "external/gtest/src/gtest")"
    mkdir -p obj/external/gtest/src/gtest/src
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/gtest/src/gtest/src/gtest-filepath.cc -o obj/external/gtest/src/gtest/src/gtest-filepath.o  -iquote external/gtest/src/gtest -iquote external/gtest/include/gtest:gtest -iquote external/gtest/include  $systemIncludes
# compile external/gtest/src/gtest
    systemIncludes="$(setupSystemIncludes "external/gtest/src/gtest")"
    mkdir -p obj/external/gtest/src/gtest/src
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/gtest/src/gtest/src/gtest-death-test.cc -o obj/external/gtest/src/gtest/src/gtest-death-test.o  -iquote external/gtest/src/gtest -iquote external/gtest/include/gtest:gtest -iquote external/gtest/include  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/nodebuilder.cpp -o obj/external/yaml-cpp/src/yaml-cpp/nodebuilder.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/tag.cpp -o obj/external/yaml-cpp/src/yaml-cpp/tag.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/emit.cpp -o obj/external/yaml-cpp/src/yaml-cpp/emit.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/nodeevents.cpp -o obj/external/yaml-cpp/src/yaml-cpp/nodeevents.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/scantoken.cpp -o obj/external/yaml-cpp/src/yaml-cpp/scantoken.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/null.cpp -o obj/external/yaml-cpp/src/yaml-cpp/null.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/scantag.cpp -o obj/external/yaml-cpp/src/yaml-cpp/scantag.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/singledocparser.cpp -o obj/external/yaml-cpp/src/yaml-cpp/singledocparser.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/regex_yaml.cpp -o obj/external/yaml-cpp/src/yaml-cpp/regex_yaml.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/scanner.cpp -o obj/external/yaml-cpp/src/yaml-cpp/scanner.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/exp.cpp -o obj/external/yaml-cpp/src/yaml-cpp/exp.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/node_data.cpp -o obj/external/yaml-cpp/src/yaml-cpp/node_data.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/emitterutils.cpp -o obj/external/yaml-cpp/src/yaml-cpp/emitterutils.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/directives.cpp -o obj/external/yaml-cpp/src/yaml-cpp/directives.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/stream.cpp -o obj/external/yaml-cpp/src/yaml-cpp/stream.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/binary.cpp -o obj/external/yaml-cpp/src/yaml-cpp/binary.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/emitter.cpp -o obj/external/yaml-cpp/src/yaml-cpp/emitter.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/ostream_wrapper.cpp -o obj/external/yaml-cpp/src/yaml-cpp/ostream_wrapper.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/convert.cpp -o obj/external/yaml-cpp/src/yaml-cpp/convert.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/parser.cpp -o obj/external/yaml-cpp/src/yaml-cpp/parser.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/simplekey.cpp -o obj/external/yaml-cpp/src/yaml-cpp/simplekey.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/emitterstate.cpp -o obj/external/yaml-cpp/src/yaml-cpp/emitterstate.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp/contrib
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/contrib/graphbuilder.cpp -o obj/external/yaml-cpp/src/yaml-cpp/contrib/graphbuilder.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp/contrib
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/contrib/graphbuilderadapter.cpp -o obj/external/yaml-cpp/src/yaml-cpp/contrib/graphbuilderadapter.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/scanscalar.cpp -o obj/external/yaml-cpp/src/yaml-cpp/scanscalar.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/node.cpp -o obj/external/yaml-cpp/src/yaml-cpp/node.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/memory.cpp -o obj/external/yaml-cpp/src/yaml-cpp/memory.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/emitfromevents.cpp -o obj/external/yaml-cpp/src/yaml-cpp/emitfromevents.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/depthguard.cpp -o obj/external/yaml-cpp/src/yaml-cpp/depthguard.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/parse.cpp -o obj/external/yaml-cpp/src/yaml-cpp/parse.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/yaml-cpp
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/yaml-cpp")"
    mkdir -p obj/external/yaml-cpp/src/yaml-cpp
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/yaml-cpp/exceptions.cpp -o obj/external/yaml-cpp/src/yaml-cpp/exceptions.o  -iquote external/yaml-cpp/src/yaml-cpp -iquote external/yaml-cpp/src  $systemIncludes
# compile external/yaml-cpp/src/testsYaml
    systemIncludes="$(setupSystemIncludes "external/yaml-cpp/src/testsYaml")"
    mkdir -p obj/external/yaml-cpp/src/testsYaml
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/yaml-cpp/src/testsYaml/main.cpp -o obj/external/yaml-cpp/src/testsYaml/main.o  -iquote external/yaml-cpp/src/testsYaml -iquote external/yaml-cpp/src  $systemIncludes
# compile external/fmt/src/fmt
    systemIncludes="$(setupSystemIncludes "external/fmt/src/fmt")"
    mkdir -p obj/external/fmt/src/fmt
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/fmt/src/fmt/os.cc -o obj/external/fmt/src/fmt/os.o  -iquote external/fmt/src/fmt -iquote external/fmt/include/fmt:fmt -iquote external/fmt/include  $systemIncludes
# compile external/fmt/src/fmt
    systemIncludes="$(setupSystemIncludes "external/fmt/src/fmt")"
    mkdir -p obj/external/fmt/src/fmt
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/fmt/src/fmt/format.cc -o obj/external/fmt/src/fmt/format.o  -iquote external/fmt/src/fmt -iquote external/fmt/include/fmt:fmt -iquote external/fmt/include  $systemIncludes
# compile external/SelfTest/src/selfTest
    systemIncludes="$(setupSystemIncludes "external/SelfTest/src/selfTest")"
    mkdir -p obj/external/SelfTest/src/selfTest
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/SelfTest/src/selfTest/selfTest.cpp -o obj/external/SelfTest/src/selfTest/selfTest.o  -iquote external/SelfTest/src/selfTest -iquote external/SelfTest/src  $systemIncludes
# compile external/SelfTest/src/testSelfTest
    systemIncludes="$(setupSystemIncludes "external/SelfTest/src/testSelfTest")"
    mkdir -p obj/external/SelfTest/src/testSelfTest
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/SelfTest/src/testSelfTest/test.cpp -o obj/external/SelfTest/src/testSelfTest/test.o  -iquote external/SelfTest/src/testSelfTest -iquote external/SelfTest/src  $systemIncludes
# compile external/SelfTest/external/Process/src/process
    systemIncludes="$(setupSystemIncludes "external/SelfTest/external/Process/src/process")"
    mkdir -p obj/external/SelfTest/external/Process/src/process
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/SelfTest/external/Process/src/process/Process.cpp -o obj/external/SelfTest/external/Process/src/process/Process.o  -iquote external/SelfTest/external/Process/src/process -iquote external/SelfTest/external/Process/src  $systemIncludes
# compile external/SelfTest/external/Process/src/process
    systemIncludes="$(setupSystemIncludes "external/SelfTest/external/Process/src/process")"
    mkdir -p obj/external/SelfTest/external/Process/src/process
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/SelfTest/external/Process/src/process/InteractiveProcess.cpp -o obj/external/SelfTest/external/Process/src/process/InteractiveProcess.o  -iquote external/SelfTest/external/Process/src/process -iquote external/SelfTest/external/Process/src  $systemIncludes
# compile external/SelfTest/external/Process/src/testProcess
    systemIncludes="$(setupSystemIncludes "external/SelfTest/external/Process/src/testProcess")"
    mkdir -p obj/external/SelfTest/external/Process/src/testProcess
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/SelfTest/external/Process/src/testProcess/test.cpp -o obj/external/SelfTest/external/Process/src/testProcess/test.o  -iquote external/SelfTest/external/Process/src/testProcess -iquote external/SelfTest/external/Process/src  $systemIncludes
# compile external/ThreadPool/src/testThreadPool
    systemIncludes="$(setupSystemIncludes "external/ThreadPool/src/testThreadPool")"
    mkdir -p obj/external/ThreadPool/src/testThreadPool
    g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c ../external/ThreadPool/src/testThreadPool/testThreadPool.cpp -o obj/external/ThreadPool/src/testThreadPool/testThreadPool.o  -iquote external/ThreadPool/src/testThreadPool -iquote external/ThreadPool/src  $systemIncludes
# linking lib/demangle.a as static_library
    mkdir -p lib
    ld -Ur -o lib/demangle.a.o obj/src/demangle/demangle.o && ar rcs lib/demangle.a lib/demangle.a.o
# linking lib/base64.a as static_library
    mkdir -p lib
    ld -Ur -o lib/base64.a.o obj/src/base64/base64.o && ar rcs lib/base64.a lib/base64.a.o
# linking lib/sargparse.a as static_library
    mkdir -p lib
    ld -Ur -o lib/sargparse.a.o obj/external/sargparse/src/sargparse/Parameter.o obj/external/sargparse/src/sargparse/ArgumentParsing.o obj/external/sargparse/src/sargparse/man.o && ar rcs lib/sargparse.a lib/sargparse.a.o
# linking lib/gtest.a as static_library
    mkdir -p lib
    ld -Ur -o lib/gtest.a.o obj/external/gtest/src/gtest/src/gtest-port.o obj/external/gtest/src/gtest/src/gtest-typed-test.o obj/external/gtest/src/gtest/src/gtest_main.o obj/external/gtest/src/gtest/src/gtest-test-part.o obj/external/gtest/src/gtest/src/gtest-printers.o obj/external/gtest/src/gtest/src/gtest.o obj/external/gtest/src/gtest/src/gtest-filepath.o obj/external/gtest/src/gtest/src/gtest-death-test.o && ar rcs lib/gtest.a lib/gtest.a.o
# linking lib/yaml-cpp.a as static_library
    mkdir -p lib
    ld -Ur -o lib/yaml-cpp.a.o obj/external/yaml-cpp/src/yaml-cpp/nodebuilder.o obj/external/yaml-cpp/src/yaml-cpp/tag.o obj/external/yaml-cpp/src/yaml-cpp/emit.o obj/external/yaml-cpp/src/yaml-cpp/nodeevents.o obj/external/yaml-cpp/src/yaml-cpp/scantoken.o obj/external/yaml-cpp/src/yaml-cpp/null.o obj/external/yaml-cpp/src/yaml-cpp/scantag.o obj/external/yaml-cpp/src/yaml-cpp/singledocparser.o obj/external/yaml-cpp/src/yaml-cpp/regex_yaml.o obj/external/yaml-cpp/src/yaml-cpp/scanner.o obj/external/yaml-cpp/src/yaml-cpp/exp.o obj/external/yaml-cpp/src/yaml-cpp/node_data.o obj/external/yaml-cpp/src/yaml-cpp/emitterutils.o obj/external/yaml-cpp/src/yaml-cpp/directives.o obj/external/yaml-cpp/src/yaml-cpp/stream.o obj/external/yaml-cpp/src/yaml-cpp/binary.o obj/external/yaml-cpp/src/yaml-cpp/emitter.o obj/external/yaml-cpp/src/yaml-cpp/ostream_wrapper.o obj/external/yaml-cpp/src/yaml-cpp/convert.o obj/external/yaml-cpp/src/yaml-cpp/parser.o obj/external/yaml-cpp/src/yaml-cpp/simplekey.o obj/external/yaml-cpp/src/yaml-cpp/emitterstate.o obj/external/yaml-cpp/src/yaml-cpp/contrib/graphbuilder.o obj/external/yaml-cpp/src/yaml-cpp/contrib/graphbuilderadapter.o obj/external/yaml-cpp/src/yaml-cpp/scanscalar.o obj/external/yaml-cpp/src/yaml-cpp/node.o obj/external/yaml-cpp/src/yaml-cpp/memory.o obj/external/yaml-cpp/src/yaml-cpp/emitfromevents.o obj/external/yaml-cpp/src/yaml-cpp/depthguard.o obj/external/yaml-cpp/src/yaml-cpp/parse.o obj/external/yaml-cpp/src/yaml-cpp/exceptions.o && ar rcs lib/yaml-cpp.a lib/yaml-cpp.a.o
# linking lib/fmt.a as static_library
    mkdir -p lib
    ld -Ur -o lib/fmt.a.o obj/external/fmt/src/fmt/os.o obj/external/fmt/src/fmt/format.o && ar rcs lib/fmt.a lib/fmt.a.o
# linking lib/process.a as static_library
    mkdir -p lib
    ld -Ur -o lib/process.a.o obj/external/SelfTest/external/Process/src/process/Process.o obj/external/SelfTest/external/Process/src/process/InteractiveProcess.o && ar rcs lib/process.a lib/process.a.o
# linking bin/testBase64 as executable
    mkdir -p bin
    g++ -rdynamic -g3 -ggdb -fdiagnostics-color=always -o bin/testBase64 obj/src/testBase64/main.o lib/base64.a
# linking bin/exampleSargparse as executable
    mkdir -p bin
    g++ -rdynamic -g3 -ggdb -fdiagnostics-color=always -o bin/exampleSargparse obj/external/sargparse/src/exampleSargparse/exampleSargparse.o obj/external/sargparse/src/exampleSargparse/exampleSargparse_Add.o obj/external/sargparse/src/exampleSargparse/exampleSargparse_NamedAdd.o obj/external/sargparse/src/exampleSargparse/exampleSargparse_Foo.o lib/sargparse.a
# linking bin/testsYaml as executable
    mkdir -p bin
    g++ -rdynamic -g3 -ggdb -fdiagnostics-color=always -o bin/testsYaml obj/external/yaml-cpp/src/testsYaml/main.o lib/yaml-cpp.a
# linking lib/selfTest.a as static_library
    mkdir -p lib
    ld -Ur -o lib/selfTest.a.o obj/external/SelfTest/src/selfTest/selfTest.o && ar rcs lib/selfTest.a lib/selfTest.a.o
# linking bin/testProcess as executable
    mkdir -p bin
    g++ -rdynamic -g3 -ggdb -fdiagnostics-color=always -o bin/testProcess obj/external/SelfTest/external/Process/src/testProcess/test.o lib/process.a -lpthread
# linking bin/testThreadPool as executable
    mkdir -p bin
    g++ -rdynamic -g3 -ggdb -fdiagnostics-color=always -o bin/testThreadPool obj/external/ThreadPool/src/testThreadPool/testThreadPool.o lib/gtest.a -lpthread
# linking bin/busy as executable
    mkdir -p bin
    g++ -rdynamic -g3 -ggdb -fdiagnostics-color=always -o bin/busy obj/src/busy/busy.o obj/src/busy/FileCache.o obj/src/busy/toolchains.o obj/src/busy/cache.o obj/src/busy/completion.o obj/src/busy/utils.o obj/src/busy/cmd/clean.o obj/src/busy/cmd/lsToolchains.o obj/src/busy/cmd/version.o obj/src/busy/cmd/status.o obj/src/busy/cmd/showPackages.o obj/src/busy/cmd/compile.o obj/src/busy/cmd/showDeps.o obj/src/busy/CompilePipe.o obj/src/busy/File.o obj/src/busy/ConsolePrinter.o obj/src/busy/analyse.o obj/src/busy/TranslationSet.o obj/src/busy/Package.o lib/base64.a lib/sargparse.a lib/yaml-cpp.a lib/fmt.a lib/process.a lib/yaml-cpp.a -lpthread
# linking bin/testSelfTest as executable
    mkdir -p bin
    g++ -rdynamic -g3 -ggdb -fdiagnostics-color=always -o bin/testSelfTest obj/external/SelfTest/src/testSelfTest/test.o lib/selfTest.a lib/process.a -lpthread
