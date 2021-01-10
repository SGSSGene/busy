#!/usr/bin/bash -e

function check {
	idx=$1
	shift
	set +e
	eval $2 >/dev/null 2>/dev/null

	r="$?"
	set -e
	if [ $r -ne "$1" ]; then
		echo "failed on:"
		echo "$2"
		echo "error code $r"
		echo ""
		echo "normal run failed on test ${idx}"
		exit -1
	fi
}


check 101 0 "./bin/exampleSargparse"
check 102 0 "./bin/exampleSargparse add"
check 103 0 "./bin/exampleSargparse my_command"
check 104 1 "./bin/exampleSargparse blub"
check 105 0 "./bin/exampleSargparse --help"
check 106 0 "./bin/exampleSargparse --bash_completion"
check 107 1 "./bin/exampleSargparse --invalid"
check 108 0 "./bin/exampleSargparse add file1 file2 -- file3 file4"
check 109 0 "./bin/exampleSargparse my_command --print_hello"
check 110 0 "./bin/exampleSargparse add --help"
check 111 0 "./bin/exampleSargparse named_add --help"
check 112 0 "./bin/exampleSargparse named_add"
check 113 0 "./bin/exampleSargparse named_add file1 file2 -- file3 file4"
check 114 0 "./bin/exampleSargparse my_command --help"
check 115 1 "./bin/exampleSargparse unknown_command"
check 116 0 "./bin/exampleSargparse unknown_command '' --bash_completion"
check 117 0 "./bin/exampleSargparse named_add '' --bash_completion"


check 120 1 "./bin/exampleSargparse --mySection.integer"
check 121 0 "./bin/exampleSargparse --mySection.integer 0"
check 122 1 "./bin/exampleSargparse --mySection.integer 0 0"
check 123 1 "./bin/exampleSargparse --mySection.integer 0.5"
check 124 1 "./bin/exampleSargparse --mySection.integer blub"

check 130 1 "./bin/exampleSargparse --mySection.string"
check 131 0 "./bin/exampleSargparse --mySection.string blub"
check 132 1 "./bin/exampleSargparse --mySection.string blub blub"

check 140 1 "./bin/exampleSargparse --mySection.double"
check 141 0 "./bin/exampleSargparse --mySection.double 0"
check 142 1 "./bin/exampleSargparse --mySection.double 0 0"
check 143 0 "./bin/exampleSargparse --mySection.double 0.5"
check 144 1 "./bin/exampleSargparse --mySection.double blub"

check 150 0 "./bin/exampleSargparse --mySection.flag"
check 151 1 "./bin/exampleSargparse --mySection.flag blub"
check 152 0 "./bin/exampleSargparse --mySection.flag true"
check 153 0 "./bin/exampleSargparse --mySection.flag false"

check 154 0 "./bin/exampleSargparse --mySection.flag my_command"
check 155 0 "./bin/exampleSargparse --mySection.flag false my_command"
check 156 0 "./bin/exampleSargparse --mySection.flag --mySection.string blubn"
check 157 0 "./bin/exampleSargparse --mySection.flag false --mySection.string blubn"

check 200 1 "./bin/exampleSargparse --mySection.file"
check 201 0 "./bin/exampleSargparse --mySection.file blub"
check 202 1 "./bin/exampleSargparse --mySection.file blub blub"

check 210 1 "./bin/exampleSargparse --mySection.path"
check 211 0 "./bin/exampleSargparse --mySection.path blub"
check 212 1 "./bin/exampleSargparse --mySection.path blub blub"

check 220 1 "./bin/exampleSargparse --mySection.cpp_file"
check 221 0 "./bin/exampleSargparse --mySection.cpp_file blub"
check 222 1 "./bin/exampleSargparse --mySection.cpp_file blub blub"

check 300 0 "./bin/exampleSargparse --mySection.multi_files"
check 301 0 "./bin/exampleSargparse --mySection.multi_files blub"
check 302 0 "./bin/exampleSargparse --mySection.multi_files blub blub"
check 303 0 "./bin/exampleSargparse --mySection.multi_files -- blub blub"

check 310 0 "./bin/exampleSargparse --mySection.multi_paths"
check 311 0 "./bin/exampleSargparse --mySection.multi_paths blub"
check 312 0 "./bin/exampleSargparse --mySection.multi_paths blub blub"
check 313 0 "./bin/exampleSargparse --mySection.multi_paths -- blub blub"

check 320 0 "./bin/exampleSargparse --mySection.multi_cpp_files"
check 321 0 "./bin/exampleSargparse --mySection.multi_cpp_files blub"
check 322 0 "./bin/exampleSargparse --mySection.multi_cpp_files blub blub"
check 323 0 "./bin/exampleSargparse --mySection.multi_cpp_files -- blub blub"

check 400 1 "./bin/exampleSargparse --my_enum"
check 401 0 "./bin/exampleSargparse --my_enum Bar"
check 402 0 "./bin/exampleSargparse --my_enum Foo"
check 403 1 "./bin/exampleSargparse --my_enum Blub"
check 404 1 "./bin/exampleSargparse --my_enum Bar Bar"
check 405 1 "./bin/exampleSargparse --my_enum Bar Foo"

check 410 1 "./bin/exampleSargparse --my_enum2"
check 411 0 "./bin/exampleSargparse --my_enum2 0"
check 412 0 "./bin/exampleSargparse --my_enum2 1"
check 413 1 "./bin/exampleSargparse --my_enum2 0 0"
check 414 1 "./bin/exampleSargparse --my_enum2 0 1"
check 415 1 "./bin/exampleSargparse --my_enum2 abc"


check 500 0 "./bin/exampleSargparse --mySection.integer 5k"
check 501 1 "./bin/exampleSargparse --mySection.integer 5pi"
check 502 1 "./bin/exampleSargparse --mySection.integer 5blub"
check 503 0 "./bin/exampleSargparse --mySection.integer 0b1000"

check 513 0 "./bin/exampleSargparse --mySection.double 1."
check 514 0 "./bin/exampleSargparse --mySection.double 1.rad"
check 514 0 "./bin/exampleSargparse --mySection.double 1.deg"
check 515 0 "./bin/exampleSargparse --mySection.double 1.pi"
check 516 0 "./bin/exampleSargparse --mySection.double 1.tau"
check 517 1 "./bin/exampleSargparse --mySection.double 1.blub"
check 518 1 "./bin/exampleSargparse --mySection.double 1.\ "
check 519 1 "./bin/exampleSargparse --mySection.double 1.\ 1."
check 520 1 "./bin/exampleSargparse --mySection.double 1.tau\ "
check 521 1 "./bin/exampleSargparse --mySection.double 1.tau\ blub"
check 522 1 "./bin/exampleSargparse --mySection.double '1.tau\ blub\ blub'"

check 1000 0 "./bin/exampleSargparse --man"
check 1001 0 "./bin/exampleSargparse add --man"
check 1002 0 "./bin/exampleSargparse my_command --man"









echo "normal tests passed"
exit 0
