#!/bin/bash
mkdir -p bootstrap.d
cd bootstrap.d

if [ ! -e "external" ]; then
    ln -s ../external external
fi
if [ ! -e "src" ]; then
    ln -s ../src src
fi
mkdir -p obj/src/busy/cmd
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/cmd/compile.cpp -o obj/src/busy/cmd/compile.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/cmd/lsToolchains.cpp -o obj/src/busy/cmd/lsToolchains.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/cmd/version.cpp -o obj/src/busy/cmd/version.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/cmd/clean.cpp -o obj/src/busy/cmd/clean.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/cmd/status.cpp -o obj/src/busy/cmd/status.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/cmd/showDeps.cpp -o obj/src/busy/cmd/showDeps.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
mkdir -p obj/src/busy
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/utils.cpp -o obj/src/busy/utils.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/busy.cpp -o obj/src/busy/busy.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/completion.cpp -o obj/src/busy/completion.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/ConsolePrinter.cpp -o obj/src/busy/ConsolePrinter.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/analyse.cpp -o obj/src/busy/analyse.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/Project.cpp -o obj/src/busy/Project.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/cache.cpp -o obj/src/busy/cache.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/File.cpp -o obj/src/busy/File.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/toolchains.cpp -o obj/src/busy/toolchains.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/FileCache.cpp -o obj/src/busy/FileCache.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
mkdir -p obj/src/busy/utils
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/utils/utils.cpp -o obj/src/busy/utils/utils.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
mkdir -p obj/src/busy
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/Package.cpp -o obj/src/busy/Package.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/busy/CompilePipe.cpp -o obj/src/busy/CompilePipe.o  -I src/busy -I src  -isystem src -isystem external/SelfTest/external/Process/src -isystem external/sargparse/src -isystem external/fmt/src -isystem external/fmt/include -isystem external/yaml-cpp/src
mkdir -p obj/src/base64
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/base64/base64.cpp -o obj/src/base64/base64.o  -I src/base64 -I src
mkdir -p obj/src/testBase64
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c src/testBase64/main.cpp -o obj/src/testBase64/main.o  -I src/testBase64 -I src  -isystem src
mkdir -p obj/external/ThreadPool/src/testThreadPool
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/ThreadPool/src/testThreadPool/testThreadPool.cpp -o obj/external/ThreadPool/src/testThreadPool/testThreadPool.o  -I external/ThreadPool/src/testThreadPool -I external/ThreadPool/src  -isystem external/ThreadPool/src -isystem external/ThreadPool/external/gtest/src -isystem external/ThreadPool/external/gtest/include
mkdir -p obj/external/ThreadPool/external/gtest/src/gtest/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/ThreadPool/external/gtest/src/gtest/src/gtest-printers.cc -o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest-printers.o  -I external/ThreadPool/external/gtest/src/gtest -I external/ThreadPool/external/gtest/include -I external/ThreadPool/external/gtest
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/ThreadPool/external/gtest/src/gtest/src/gtest-death-test.cc -o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest-death-test.o  -I external/ThreadPool/external/gtest/src/gtest -I external/ThreadPool/external/gtest/include -I external/ThreadPool/external/gtest
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/ThreadPool/external/gtest/src/gtest/src/gtest_main.cpp -o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest_main.o  -I external/ThreadPool/external/gtest/src/gtest -I external/ThreadPool/external/gtest/include -I external/ThreadPool/external/gtest
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/ThreadPool/external/gtest/src/gtest/src/gtest-test-part.cc -o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest-test-part.o  -I external/ThreadPool/external/gtest/src/gtest -I external/ThreadPool/external/gtest/include -I external/ThreadPool/external/gtest
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/ThreadPool/external/gtest/src/gtest/src/gtest-typed-test.cc -o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest-typed-test.o  -I external/ThreadPool/external/gtest/src/gtest -I external/ThreadPool/external/gtest/include -I external/ThreadPool/external/gtest
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/ThreadPool/external/gtest/src/gtest/src/gtest-port.cc -o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest-port.o  -I external/ThreadPool/external/gtest/src/gtest -I external/ThreadPool/external/gtest/include -I external/ThreadPool/external/gtest
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/ThreadPool/external/gtest/src/gtest/src/gtest-filepath.cc -o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest-filepath.o  -I external/ThreadPool/external/gtest/src/gtest -I external/ThreadPool/external/gtest/include -I external/ThreadPool/external/gtest
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/ThreadPool/external/gtest/src/gtest/src/gtest.cc -o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest.o  -I external/ThreadPool/external/gtest/src/gtest -I external/ThreadPool/external/gtest/include -I external/ThreadPool/external/gtest
mkdir -p obj/external/SelfTest/src/selfTest
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/SelfTest/src/selfTest/selfTest.cpp -o obj/external/SelfTest/src/selfTest/selfTest.o  -I external/SelfTest/src/selfTest -I external/SelfTest/src  -isystem external/SelfTest/external/Process/src
mkdir -p obj/external/SelfTest/src/testSelfTest
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/SelfTest/src/testSelfTest/test.cpp -o obj/external/SelfTest/src/testSelfTest/test.o  -I external/SelfTest/src/testSelfTest -I external/SelfTest/src  -isystem external/SelfTest/src -isystem external/SelfTest/external/Process/src
mkdir -p obj/external/SelfTest/external/Process/src/process
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/SelfTest/external/Process/src/process/InteractiveProcess.cpp -o obj/external/SelfTest/external/Process/src/process/InteractiveProcess.o  -I external/SelfTest/external/Process/src/process -I external/SelfTest/external/Process/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/SelfTest/external/Process/src/process/Process.cpp -o obj/external/SelfTest/external/Process/src/process/Process.o  -I external/SelfTest/external/Process/src/process -I external/SelfTest/external/Process/src
mkdir -p obj/external/SelfTest/external/Process/src/testProcess
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/SelfTest/external/Process/src/testProcess/test.cpp -o obj/external/SelfTest/external/Process/src/testProcess/test.o  -I external/SelfTest/external/Process/src/testProcess -I external/SelfTest/external/Process/src  -isystem external/SelfTest/external/Process/src
mkdir -p obj/external/sargparse/src/sargparse
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/sargparse/src/sargparse/ArgumentParsing.cpp -o obj/external/sargparse/src/sargparse/ArgumentParsing.o  -I external/sargparse/src/sargparse -I external/sargparse/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/sargparse/src/sargparse/man.cpp -o obj/external/sargparse/src/sargparse/man.o  -I external/sargparse/src/sargparse -I external/sargparse/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/sargparse/src/sargparse/Parameter.cpp -o obj/external/sargparse/src/sargparse/Parameter.o  -I external/sargparse/src/sargparse -I external/sargparse/src
mkdir -p obj/external/fmt/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/fmt/src/os.cc -o obj/external/fmt/src/os.o  -I external/fmt/src/fmt -I external/fmt/include -I external/fmt
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/fmt/src/format.cc -o obj/external/fmt/src/format.o  -I external/fmt/src/fmt -I external/fmt/include -I external/fmt
mkdir -p obj/external/yaml-cpp/src/yaml-cpp
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/binary.cpp -o obj/external/yaml-cpp/src/yaml-cpp/binary.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/depthguard.cpp -o obj/external/yaml-cpp/src/yaml-cpp/depthguard.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/stream.cpp -o obj/external/yaml-cpp/src/yaml-cpp/stream.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/simplekey.cpp -o obj/external/yaml-cpp/src/yaml-cpp/simplekey.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/scantag.cpp -o obj/external/yaml-cpp/src/yaml-cpp/scantag.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/emit.cpp -o obj/external/yaml-cpp/src/yaml-cpp/emit.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/emitterstate.cpp -o obj/external/yaml-cpp/src/yaml-cpp/emitterstate.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/regex_yaml.cpp -o obj/external/yaml-cpp/src/yaml-cpp/regex_yaml.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/tag.cpp -o obj/external/yaml-cpp/src/yaml-cpp/tag.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/scantoken.cpp -o obj/external/yaml-cpp/src/yaml-cpp/scantoken.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/memory.cpp -o obj/external/yaml-cpp/src/yaml-cpp/memory.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/null.cpp -o obj/external/yaml-cpp/src/yaml-cpp/null.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/parse.cpp -o obj/external/yaml-cpp/src/yaml-cpp/parse.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/emitter.cpp -o obj/external/yaml-cpp/src/yaml-cpp/emitter.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/exp.cpp -o obj/external/yaml-cpp/src/yaml-cpp/exp.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/parser.cpp -o obj/external/yaml-cpp/src/yaml-cpp/parser.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/emitterutils.cpp -o obj/external/yaml-cpp/src/yaml-cpp/emitterutils.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/directives.cpp -o obj/external/yaml-cpp/src/yaml-cpp/directives.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/exceptions.cpp -o obj/external/yaml-cpp/src/yaml-cpp/exceptions.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/singledocparser.cpp -o obj/external/yaml-cpp/src/yaml-cpp/singledocparser.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/node_data.cpp -o obj/external/yaml-cpp/src/yaml-cpp/node_data.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/nodebuilder.cpp -o obj/external/yaml-cpp/src/yaml-cpp/nodebuilder.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/emitfromevents.cpp -o obj/external/yaml-cpp/src/yaml-cpp/emitfromevents.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/convert.cpp -o obj/external/yaml-cpp/src/yaml-cpp/convert.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/scanner.cpp -o obj/external/yaml-cpp/src/yaml-cpp/scanner.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/scanscalar.cpp -o obj/external/yaml-cpp/src/yaml-cpp/scanscalar.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/ostream_wrapper.cpp -o obj/external/yaml-cpp/src/yaml-cpp/ostream_wrapper.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/nodeevents.cpp -o obj/external/yaml-cpp/src/yaml-cpp/nodeevents.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
mkdir -p obj/external/yaml-cpp/src/yaml-cpp/contrib
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/contrib/graphbuilder.cpp -o obj/external/yaml-cpp/src/yaml-cpp/contrib/graphbuilder.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/contrib/graphbuilderadapter.cpp -o obj/external/yaml-cpp/src/yaml-cpp/contrib/graphbuilderadapter.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
mkdir -p obj/external/yaml-cpp/src/yaml-cpp
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/yaml-cpp/node.cpp -o obj/external/yaml-cpp/src/yaml-cpp/node.o  -I external/yaml-cpp/src/yaml-cpp -I external/yaml-cpp/src
mkdir -p obj/external/yaml-cpp/src/testsYaml
g++ -O0 -std=c++20 -fPIC -MD -g3 -ggdb -fdiagnostics-color=always -c external/yaml-cpp/src/testsYaml/main.cpp -o obj/external/yaml-cpp/src/testsYaml/main.o  -I external/yaml-cpp/src/testsYaml -I external/yaml-cpp/src  -isystem external/yaml-cpp/src
mkdir -p lib
ld -r -o lib/base64.a.o obj/src/base64/base64.o && ar rcs lib/base64.a lib/base64.a.o
ld -r -o lib/gtest.a.o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest-printers.o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest-death-test.o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest_main.o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest-test-part.o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest-typed-test.o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest-port.o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest-filepath.o obj/external/ThreadPool/external/gtest/src/gtest/src/gtest.o && ar rcs lib/gtest.a lib/gtest.a.o
ld -r -o lib/process.a.o obj/external/SelfTest/external/Process/src/process/InteractiveProcess.o obj/external/SelfTest/external/Process/src/process/Process.o && ar rcs lib/process.a lib/process.a.o
ld -r -o lib/sargparse.a.o obj/external/sargparse/src/sargparse/ArgumentParsing.o obj/external/sargparse/src/sargparse/man.o obj/external/sargparse/src/sargparse/Parameter.o && ar rcs lib/sargparse.a lib/sargparse.a.o
ld -r -o lib/fmt.a.o obj/external/fmt/src/os.o obj/external/fmt/src/format.o && ar rcs lib/fmt.a lib/fmt.a.o
ld -r -o lib/yaml-cpp.a.o obj/external/yaml-cpp/src/yaml-cpp/binary.o obj/external/yaml-cpp/src/yaml-cpp/depthguard.o obj/external/yaml-cpp/src/yaml-cpp/stream.o obj/external/yaml-cpp/src/yaml-cpp/simplekey.o obj/external/yaml-cpp/src/yaml-cpp/scantag.o obj/external/yaml-cpp/src/yaml-cpp/emit.o obj/external/yaml-cpp/src/yaml-cpp/emitterstate.o obj/external/yaml-cpp/src/yaml-cpp/regex_yaml.o obj/external/yaml-cpp/src/yaml-cpp/tag.o obj/external/yaml-cpp/src/yaml-cpp/scantoken.o obj/external/yaml-cpp/src/yaml-cpp/memory.o obj/external/yaml-cpp/src/yaml-cpp/null.o obj/external/yaml-cpp/src/yaml-cpp/parse.o obj/external/yaml-cpp/src/yaml-cpp/emitter.o obj/external/yaml-cpp/src/yaml-cpp/exp.o obj/external/yaml-cpp/src/yaml-cpp/parser.o obj/external/yaml-cpp/src/yaml-cpp/emitterutils.o obj/external/yaml-cpp/src/yaml-cpp/directives.o obj/external/yaml-cpp/src/yaml-cpp/exceptions.o obj/external/yaml-cpp/src/yaml-cpp/singledocparser.o obj/external/yaml-cpp/src/yaml-cpp/node_data.o obj/external/yaml-cpp/src/yaml-cpp/nodebuilder.o obj/external/yaml-cpp/src/yaml-cpp/emitfromevents.o obj/external/yaml-cpp/src/yaml-cpp/convert.o obj/external/yaml-cpp/src/yaml-cpp/scanner.o obj/external/yaml-cpp/src/yaml-cpp/scanscalar.o obj/external/yaml-cpp/src/yaml-cpp/ostream_wrapper.o obj/external/yaml-cpp/src/yaml-cpp/nodeevents.o obj/external/yaml-cpp/src/yaml-cpp/contrib/graphbuilder.o obj/external/yaml-cpp/src/yaml-cpp/contrib/graphbuilderadapter.o obj/external/yaml-cpp/src/yaml-cpp/node.o && ar rcs lib/yaml-cpp.a lib/yaml-cpp.a.o
mkdir -p bin
g++ -rdynamic -g3 -ggdb -fdiagnostics-color=always -o bin/testBase64 obj/src/testBase64/main.o lib/base64.a
g++ -rdynamic -g3 -ggdb -fdiagnostics-color=always -o bin/testThreadPool obj/external/ThreadPool/src/testThreadPool/testThreadPool.o lib/gtest.a -lpthread
mkdir -p lib
ld -Ur -o lib/selfTest.a.o obj/external/SelfTest/src/selfTest/selfTest.o && ar rcs lib/selfTest.a lib/selfTest.a.o
mkdir -p bin
g++ -rdynamic -g3 -ggdb -fdiagnostics-color=always -o bin/testProcess obj/external/SelfTest/external/Process/src/testProcess/test.o lib/process.a -lpthread
g++ -rdynamic -g3 -ggdb -fdiagnostics-color=always -o bin/testsYaml obj/external/yaml-cpp/src/testsYaml/main.o lib/yaml-cpp.a
g++ -rdynamic -g3 -ggdb -fdiagnostics-color=always -o bin/testSelfTest obj/external/SelfTest/src/testSelfTest/test.o lib/selfTest.a lib/process.a -lpthread
g++ -rdynamic -g3 -ggdb -fdiagnostics-color=always -o bin/busy obj/src/busy/cmd/compile.o obj/src/busy/cmd/lsToolchains.o obj/src/busy/cmd/version.o obj/src/busy/cmd/clean.o obj/src/busy/cmd/status.o obj/src/busy/cmd/showDeps.o obj/src/busy/utils.o obj/src/busy/busy.o obj/src/busy/completion.o obj/src/busy/ConsolePrinter.o obj/src/busy/analyse.o obj/src/busy/Project.o obj/src/busy/cache.o obj/src/busy/File.o obj/src/busy/toolchains.o obj/src/busy/FileCache.o obj/src/busy/utils/utils.o obj/src/busy/Package.o obj/src/busy/CompilePipe.o lib/base64.a lib/process.a lib/sargparse.a lib/fmt.a lib/yaml-cpp.a -lstdc++fs -lpthread
