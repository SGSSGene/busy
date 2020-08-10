#!/bin/bash

if [ ! -e bin/busy ]; then
	echo "failed - bin/busy not found"
	echo "please execute this file from any busy build directory"
	exit 1
fi
BUSY="$(pwd)/bin/busy"

cd "${0%/*}"


( # check normal compilation of executable works
	rm -rf testProject/build
	mkdir -p testProject/build
	cd testProject/build
	$BUSY ../busy.yaml > /dev/null

	if [ "$(bin/app)" != "Hello World" ]; then
		echo "failed"
		exit 1
	fi
	cd ../..
	rm -rf testProject/build
)


( # check compilation works under different build folder
	rm -rf testProject/different-build
	mkdir -p testProject/different-build
	cd testProject/different-build
	$BUSY ../busy.yaml > /dev/null

	if [ "$(bin/app)" != "Hello World" ]; then
		echo "failed"
		exit 1
	fi
	cd ../..
	rm -rf testProject/different-build
)

( # check compilation works outside of project path
	rm -rf external-build
	mkdir -p external-build
	cd external-build
	$BUSY ../testProject/busy.yaml > /dev/null

	if [ "$(bin/app)" != "Hello World" ]; then
		echo "failed"
		exit 1
	fi
	cd ..
	rm -rf external-build
)

( # check normal compilation of executable works  with build path given
	rm -rf testProject/build
	$BUSY --build testProject/build testProject/busy.yaml > /dev/null

	if [ "$(testProject/build/bin/app)" != "Hello World" ]; then
		echo "failed"
		exit 1
	fi
	rm -rf testProject/build
)


( # check compilation works under different build folder with build path given
	rm -rf testProject/different-build
	$BUSY --build testProject/different-build testProject/busy.yaml > /dev/null

	if [ "$(testProject/different-build/bin/app)" != "Hello World" ]; then
		echo "failed"
		exit 1
	fi
	rm -rf testProject/different-build
)


( # check compilation works outside of project path with build path given
	rm -rf external-build
	$BUSY --build external-build testProject/busy.yaml > /dev/null

	if [ "$(external-build/bin/app)" != "Hello World" ]; then
		echo "failed"
		exit 1
	fi
	rm -rf external-build
)


