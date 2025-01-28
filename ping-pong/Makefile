CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -g
CLIBS=-lpthread

PROCESS_SRC=./process/main.c
PROCESS_TARGET=multiprocess-ping-pong

THREAD_SRC=./thread/main.c
THREAD_TARGET=multithread-ping-pong

SHARED_SRC=./shared/src/safe-sem.c
SHARED_INCLUDE=./shared/include/

BINARY_DIR=./bin/

all:
	@echo "Use \"make process\" or \"make thread\"."

dir:
	@mkdir -p $(BINARY_DIR)

process: dir
	$(CC) $(CFLAGS) $(CLIBS) $(PROCESS_SRC) $(SHARED_SRC) -I$(SHARED_INCLUDE) -o $(BINARY_DIR)$(PROCESS_TARGET)

process_memcheck: process
	@valgrind --leak-check=full --track-origins=yes $(BINARY_DIR)$(PROCESS_TARGET)

thread: dir
	$(CC) $(CFLAGS) $(CLIBS) $(THREAD_SRC) $(SHARED_SRC) -I$(SHARED_INCLUDE) -o $(BINARY_DIR)$(THREAD_TARGET)

thread_memcheck: thread
	@valgrind --tool=helgrind --history-level=approx $(BINARY_DIR)$(THREAD_TARGET)

clean:
	$(RM) -r $(BINARY_DIR)

.PHONY: thread process clean dir