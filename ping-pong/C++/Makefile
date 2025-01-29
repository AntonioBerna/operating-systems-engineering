CC=clang++
CFLAGS=-Wall -Wextra -Werror -pedantic -O1 -g
CLIBS=-lpthread

SRCS=./thread/main.cpp
TARGET=multithread-ping-pong

BINARY_DIR=./bin/

all:
	@mkdir -p $(BINARY_DIR)
	$(CC) $(CFLAGS) $(CLIBS) $(SRCS) -o $(BINARY_DIR)$(TARGET)

clean:
	$(RM) -r $(BINARY_DIR)

.PHONY: clean