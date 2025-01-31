CC=clang++
CFLAGS=-fsanitize=thread -Wall -Wextra -Werror -pedantic -pthread

SRCS=./src/main.cpp
TARGET=multithread-ping-pong

BINARY_DIR=./bin/

all:
	@mkdir -p $(BINARY_DIR)
	$(CC) $(CFLAGS) $(SRCS) -o $(BINARY_DIR)$(TARGET)

clean:
	$(RM) -r $(BINARY_DIR)

.PHONY: clean