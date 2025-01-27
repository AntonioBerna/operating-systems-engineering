#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>

static size_t iterations;

typedef struct {
    pthread_t thread;
    void *(*start_routine)(void *);
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} thread_t;

#define NUM_THREADS 2
static thread_t threads[NUM_THREADS];

static void *ping(void *arg) {
    size_t id = (size_t)arg;
    for (size_t i = 0; i < iterations;) {
        pthread_mutex_lock(&threads[id].mutex);
        printf("(%ld) ping\n", ++i);
        pthread_cond_signal(&threads[(id + 1) % NUM_THREADS].cond);
        pthread_cond_wait(&threads[id].cond, &threads[id].mutex);
        pthread_mutex_unlock(&threads[id].mutex);
    }
    return NULL;
}

static void *pong(void *arg) {
    size_t id = (size_t)arg;
    for (size_t i = 0; i < iterations;) {
        pthread_mutex_lock(&threads[id].mutex);
        pthread_cond_wait(&threads[id].cond, &threads[id].mutex);
        printf("(%ld) pong\n", ++i);
        pthread_cond_signal(&threads[(id + 1) % NUM_THREADS].cond);
        pthread_mutex_unlock(&threads[id].mutex);
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
        pthread_mutex_init(&threads[i].mutex, NULL);
        pthread_cond_init(&threads[i].cond, NULL);
        pthread_create(&threads[i].thread, NULL, threads[i].start_routine, (void *)i);
    }

    for (size_t i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i].thread, NULL);
    }

    for (size_t i = 0; i < NUM_THREADS; ++i) {
        pthread_mutex_destroy(&threads[i].mutex);
        pthread_cond_destroy(&threads[i].cond);
    }

    return 0;
}
