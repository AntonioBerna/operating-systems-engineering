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
    const char *message;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool ready;
} thread_t;

#define NUM_THREADS 2
static thread_t threads[NUM_THREADS];

static void cleanup(void) {
    for (size_t i = 0; i < NUM_THREADS; ++i) {
        pthread_mutex_destroy(&threads[i].mutex);
        pthread_cond_destroy(&threads[i].cond);
    }
}

static void *worker(void *arg) {
    size_t id = (size_t)arg;
    size_t next = (id + 1) % NUM_THREADS;
    
    for (size_t i = 0; i < iterations;) {
        pthread_mutex_lock(&threads[id].mutex);
        while (!threads[id].ready) {
            pthread_cond_wait(&threads[id].cond, &threads[id].mutex);
        }
        printf("(%zu) %s\n", ++i, threads[id].message);
        threads[id].ready = false;
        pthread_mutex_unlock(&threads[id].mutex);

        pthread_mutex_lock(&threads[next].mutex);
        threads[next].ready = true;
        pthread_cond_signal(&threads[next].cond);
        pthread_mutex_unlock(&threads[next].mutex);
    }
    return NULL;
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

    for (size_t i = 0; i < NUM_THREADS; ++i) {
        threads[i].start_routine = worker;
        threads[i].message = (i == 0) ? "ping" : "pong";
        
        if (pthread_mutex_init(&threads[i].mutex, NULL) != 0) {
            perror("pthread_mutex_init");
            cleanup();
            return 1;
        }
        
        if (pthread_cond_init(&threads[i].cond, NULL) != 0) {
            perror("pthread_cond_init");
            cleanup();
            return 1;
        }
        
        threads[i].ready = (i == 0);
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

    cleanup();
    
    return 0;
}
