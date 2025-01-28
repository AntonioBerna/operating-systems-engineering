#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "safe-sem.h"

static size_t iterations;

#define NUM_PROCESSES 2
static pid_t pids[NUM_PROCESSES];
static safe_sem_t sems[NUM_PROCESSES];

static void cleanup() {
    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        safe_sem_destroy(&sems[i]);
    }
}

static void ping(size_t id) {
    for (size_t i = 0; i < iterations;) {
        safe_sem_wait(&sems[id]);
        printf("(%zu) ping\n", ++i);
        safe_sem_post(&sems[(id + 1) % NUM_PROCESSES]);
    }
    exit(0);
}

static void pong(size_t id) {
    for (size_t i = 0; i < iterations;) {
        safe_sem_wait(&sems[id]);
        printf("(%zu) pong\n", ++i);
        safe_sem_post(&sems[(id + 1) % NUM_PROCESSES]);
    }
    exit(0);
}

static bool to_size_t(const char *buffer, size_t *value) {
    char *endptr;
    size_t val;
    if (*buffer == '-') {
        return false;
    }
    errno = 0;
    val = strtoul(buffer, &endptr, 0);
    if (errno == ERANGE || endptr == buffer || *endptr != '\0' || val > SIZE_MAX) {
        return false;
    }
    *value = (size_t)val;
    return true;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [iterations]\n", *argv);
        return 1;
    }

    if (!to_size_t(argv[1], &iterations)) {
        fprintf(stderr, "Invalid number of iterations: %s\n", argv[1]);
        return 1;
    }

    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        if (safe_sem_init(&sems[i], IPC_PRIVATE, 0, (i == 0) ? 1 : 0) == -1) {
            fprintf(stderr, "Failed to initialize semaphore %zu\n", i);
            cleanup();
            return 1;
        }
    }

    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
            cleanup();
            return 1;
        }

        if (pids[i] == 0) {
            if (i % 2 == 0) {
                ping(i);
            } else {
                pong(i);
            }
        }
    }

    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        waitpid(pids[i], NULL, 0);
    }

    cleanup();

    return 0;
}
