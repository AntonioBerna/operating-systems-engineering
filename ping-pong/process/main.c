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

static void cleanup() {
    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        if (sems[i] != SEM_FAILED) {
            sem_close(sems[i]);
            char sem_name[64];
            snprintf(sem_name, sizeof(sem_name), "/ping_pong_semaphore_%zu", i);
            sem_unlink(sem_name);
        }
    }
}

static void ping(size_t id) {
    for (size_t i = 0; i < iterations;) {
        sem_wait(sems[id]);
        printf("(%ld) ping\n", ++i);
        sem_post(sems[(id + 1) % NUM_PROCESSES]);
    }
    // Chiudi tutti i semafori
    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        sem_close(sems[i]);
    }
    exit(0); // Usa exit() invece di _exit()
}

static void pong(size_t id) {
    for (size_t i = 0; i < iterations;) {
        sem_wait(sems[id]);
        printf("(%ld) pong\n", ++i);
        sem_post(sems[(id + 1) % NUM_PROCESSES]);
    }
    // Chiudi tutti i semafori
    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        sem_close(sems[i]);
    }
    exit(0); // Usa exit() invece di _exit()
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

    // Apri i semafori
    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        char sem_name[64];
        snprintf(sem_name, sizeof(sem_name), "/ping_pong_semaphore_%zu", i);
        sems[i] = sem_open(sem_name, O_CREAT | O_EXCL, 0644, (i == 0) ? 1 : 0); // Inizializza il primo semaforo a 1
        if (sems[i] == SEM_FAILED) {
            perror("sem_open");
            cleanup();
            return 1;
        }
    }

    // Crea i processi figli
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

    // Attendi che i processi figli terminino
    for (size_t i = 0; i < NUM_PROCESSES; ++i) {
        waitpid(pids[i], NULL, 0);
    }

    // Pulisci i semafori
    cleanup();

    return 0;
}