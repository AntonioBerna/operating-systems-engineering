#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>

#include "safe-sem.h"

static size_t iterations;

typedef struct {
    pthread_t thread;
    void *(*start_routine)(void *);
    safe_sem_t sem;
} thread_t;

#define NUM_THREADS 2
static thread_t threads[NUM_THREADS];

static void cleanup() {
    for (size_t i = 0; i < NUM_THREADS; ++i) {
        safe_sem_destroy(&threads[i].sem);
    }
}

static void *ping(void *arg) {
    size_t id = (size_t)arg;
    size_t next = (id + 1) % NUM_THREADS;
    
    for (size_t i = 0; i < iterations;) {
        safe_sem_wait(&threads[id].sem);
        printf("(%zu) ping\n", ++i);
        safe_sem_post(&threads[next].sem);
    }
    return NULL;
}

static void *pong(void *arg) {
    size_t id = (size_t)arg;
    size_t next = (id + 1) % NUM_THREADS;
    
    for (size_t i = 0; i < iterations;) {
        safe_sem_wait(&threads[id].sem);
        printf("(%zu) pong\n", ++i);
        safe_sem_post(&threads[next].sem);
    }
    return NULL;
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

    for (size_t i = 0; i < NUM_THREADS; ++i) {
        threads[i].start_routine = (i & 1) == 0 ? ping : pong;
        if (safe_sem_init(&threads[i].sem, IPC_PRIVATE, 0, i == 0 ? 1 : 0) == -1) {
            fprintf(stderr, "Failed to initialize semaphore %zu\n", i);
            cleanup();
            return 1;
        }
    }

    for (size_t i = 0; i < NUM_THREADS; ++i) {
        if (pthread_create(&threads[i].thread, NULL, threads[i].start_routine, (void *)i) != 0) {
            perror("pthread_create");
            cleanup();
            return 1;
        }
    }

    for (size_t i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i].thread, NULL);
    }

    for (size_t i = 0; i < NUM_THREADS; ++i) {
        safe_sem_destroy(&threads[i].sem);
    }

    return 0;
}
