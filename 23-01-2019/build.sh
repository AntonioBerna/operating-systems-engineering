#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -ggdb"
LIBS="-lpthread"

mkdir -p ./build/
clang $CFLAGS -o ./build/prog prog.c $LIBS

