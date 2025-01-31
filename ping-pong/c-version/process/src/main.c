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

typedef struct {
    pid_t pid;
    const char *message;
    safe_sem_t sem;
    safe_sem_t *next_sem;
} process_t;

#define NUM_PROCESSES 2
static process_t processes[NUM_PROCESSES];

static void cleanup(void) {
    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        safe_sem_destroy(&processes[i].sem);
    }
}

static void worker(size_t id) {
    process_t *proc = &processes[id];
    for (size_t i = 0; i < iterations;) {
        safe_sem_wait(&proc->sem);
        printf("(%zu) %s\n", ++i, proc->message);
        safe_sem_post(proc->next_sem);
    }
    exit(EXIT_SUCCESS);
}

#define BASE 10
static bool to_size_t(const char *buffer, size_t *value) {
    char *endptr;
    if (*buffer == '-') return false;
    errno = 0;
    *value = strtoul(buffer, &endptr, BASE);
    return !(errno == ERANGE || endptr == buffer || *endptr != '\0' || *value > SIZE_MAX);
}
#undef BASE

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [iterations]\n", argv[0]);
        return 1;
    }

    if (!to_size_t(argv[1], &iterations)) {
        fprintf(stderr, "Invalid number of iterations: %s\n", argv[1]);
        return 1;
    }

    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        int initial_value = (i == 0) ? 1 : 0;
        if (safe_sem_init(&processes[i].sem, IPC_PRIVATE, 0, initial_value) == -1) {
            fprintf(stderr, "Failed to initialize semaphore %zu\n", i);
            cleanup();
            return 1;
        }
        processes[i].next_sem = &processes[(i + 1) % NUM_PROCESSES].sem;
        processes[i].message = ((i & 1) == 0) ? "ping" : "pong";
    }

    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        processes[i].pid = fork();
        if (processes[i].pid == -1) {
            perror("fork");
            cleanup();
            return 1;
        } else if (processes[i].pid == 0) {
            worker(i);
        }
    }

    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        waitpid(processes[i].pid, NULL, 0);
    }

    cleanup();

    return 0;
}
