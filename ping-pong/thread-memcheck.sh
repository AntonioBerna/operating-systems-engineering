#!/bin/bash

for i in {1..10};
do
    valgrind --tool=helgrind --history-level=approx ./bin/multithread-ping-pong "$i";
done