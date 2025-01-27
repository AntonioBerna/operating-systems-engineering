#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>

static size_t iterations;

#define NUM_PROCESSES 2
static pid_t pids[NUM_PROCESSES];
static sem_t *sems[NUM_PROCESSES];

static void ping(size_t id) {
    for (size_t i = 0; i < iterations;) {
        sem_wait(sems[id]);
        printf("(%ld) ping\n", ++i);
        sem_post(sems[(id + 1) % NUM_PROCESSES]);
    }
}

static void pong(size_t id) {
    for (size_t i = 0; i < iterations;) {
        sem_wait(sems[id]);
        printf("(%ld) pong\n", ++i);
        sem_post(sems[(id + 1) % NUM_PROCESSES]);
    }
}

static bool to_size_t(const char *buffer, size_t *value) {
    char *endptr;
    errno = 0;
    *value = (size_t)strtol(buffer, &endptr, 0);
    return !(errno == ERANGE || endptr == buffer || (*endptr && *endptr != '\n') || *value == SIZE_MAX);
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
        char sem_name[64];
        snprintf(sem_name, sizeof(sem_name), "/ping_pong_semaphore_%zu", i);
        sems[i] = sem_open(sem_name, O_CREAT, 0644, (i == 0) ? 0 : 0);
        if (sems[i] == SEM_FAILED) {
            perror("sem_open");
            return 1;
        }
    }

    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("fork");
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

    // Start the ping-pong game.
    sem_post(sems[0]);

    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        waitpid(pids[i], NULL, 0);
    }

    // Use `lsof | grep sem` command to check the semaphore status.
    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        char sem_name[64];
        snprintf(sem_name, sizeof(sem_name), "/ping_pong_semaphore_%zu", i);
        sem_close(sems[i]);
        sem_unlink(sem_name);
    }

    return 0;
}
