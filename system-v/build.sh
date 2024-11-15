#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -ggdb"
LIBS="-lpthread"

mkdir -p ./build/
gcc $CFLAGS -o ./build/prog main.c $LIBS -DDEBUG
