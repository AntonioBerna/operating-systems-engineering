#!/bin/bash

if [ ! -d "./bin" ]; then
    echo "Please compile the code first.";
    exit 1;
fi

for i in {1..10};
do
    valgrind --tool=memcheck --leak-check=full --track-origins=yes ./bin/multiprocess-ping-pong "$i";
done
