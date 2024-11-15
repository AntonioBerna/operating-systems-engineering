#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -ggdb"
LIBS="-lpthread"

mkdir -p ./build/

if [ -z "$1" ]; then
	echo "Building in release mode"
	clang $CFLAGS -o ./build/prog prog.c $LIBS
	exit 0
fi

if [ "$1" = "debug" ]; then
	echo "Building in debug mode"
	clang $CFLAGS -o ./build/prog prog.c $LIBS -DDEBUG
	exit 0
fi
