#!/bin/bash

gcc -Wall -Wextra -Werror -pedantic -g -lpthread main.c

for i in {1..10}
do
    valgrind --tool=helgrind --history-level=approx ./a.out $i
done