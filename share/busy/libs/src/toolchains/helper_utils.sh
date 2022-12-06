#!/bin/bash

function parseMode {
    var="$3"
    if [ "${1}" = "${2}" ]; then
        parseMode="${var}"
        eval ${var}="${!var:-()}"
        eval set_${var}=1
        r="1"
    fi
}
function parseValue {
    if [ -n "${parseMode-}" ]; then
        eval "${parseMode}+=(${1})"
        r="1"
    fi
}
function parse {
    # collect all options
    arguments=()
    while [ "$1" != "--" ]; do
        arguments+=("$1")
        shift
    done
    # parse rest of the argument line
    while [ $# -gt 0 ]; do
        shift
        r=
        for arg in "${arguments[@]}"; do
            if [ -z "${r}" ]; then
                parseMode "${1-}" ${arg}
            fi
        done
        if [ -z "$r" ]; then
            parseValue "${1-}"
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

function parseDepFile {
    file="$1"
    shift

    cat ${file} | \
        awk '{
            split($0, a, " ");
            for (key in a) {
                x=a[key];
                if (x != "\\") {
                    print "  - " x
                }
            }
        }' | tail -n +2 | sort
}


