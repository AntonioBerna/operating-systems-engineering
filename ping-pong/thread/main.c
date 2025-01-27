#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>

static size_t iterations;

typedef struct {
    pthread_t thread;
    void *(*start_routine)(void *);
    sem_t sem;
} thread_t;

#define NUM_THREADS 2
static thread_t threads[NUM_THREADS];

static void *ping(void *arg) {
    size_t id = (size_t)arg;
    size_t next = (id + 1) % NUM_THREADS;
    
    for (size_t i = 0; i < iterations;) {
        sem_wait(&threads[id].sem);
        printf("(%ld) ping\n", ++i);
        sem_post(&threads[next].sem);
    }
    return NULL;
}

static void *pong(void *arg) {
    size_t id = (size_t)arg;
    size_t next = (id + 1) % NUM_THREADS;
    
    for (size_t i = 0; i < iterations;) {
        sem_wait(&threads[id].sem);
        printf("(%ld) pong\n", ++i);
        sem_post(&threads[next].sem);
    }
    return NULL;
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

    for (size_t i = 0; i < NUM_THREADS; ++i) {
        threads[i].start_routine = (i & 1) == 0 ? ping : pong;
        sem_init(&threads[i].sem, 0, i == 0 ? 1 : 0);
    }

    for (size_t i = 0; i < NUM_THREADS; ++i) {
        pthread_create(&threads[i].thread, NULL, threads[i].start_routine, (void *)i);
    }

    for (size_t i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i].thread, NULL);
    }

    for (size_t i = 0; i < NUM_THREADS; ++i) {
        sem_destroy(&threads[i].sem);
    }

    return 0;
}