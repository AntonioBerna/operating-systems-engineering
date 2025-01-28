#!/bin/bash

for i in {1..10}
do
    valgrind --leak-check=full --track-origins=yes ./a.out $i
done