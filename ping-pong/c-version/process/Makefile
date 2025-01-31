CC=clang
CFLAGS=-Wall -Wextra -Werror -pedantic

SRCS=./src/main.c
TARGET=multiprocess-ping-pong

LIB_SRCS=./lib/safe-sem/src/safe-sem.c
LIB_INCLUDE=./lib/safe-sem/include/

BINARY_DIR=./bin/

all:
	@mkdir -p $(BINARY_DIR)
	$(CC) $(CFLAGS) $(SRCS) $(LIB_SRCS) -I$(LIB_INCLUDE) -o $(BINARY_DIR)$(TARGET)

clean:
	$(RM) -r $(BINARY_DIR)

.PHONY: clean