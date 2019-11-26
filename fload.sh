#!/bin/bash

#Just a bash script to add a full directory to the filesystem

function run() {
	./filefs -a "${file}" -f fs
}

function traverse() {
for file in "$1"/*
do
    if [ ! -d "${file}" ] ; then
        run "${file}"
    else
        traverse "${file}"
    fi
done
}

function main() {
    traverse "$1"
}

main "$1"
