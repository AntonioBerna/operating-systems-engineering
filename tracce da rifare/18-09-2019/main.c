#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 10

int primes[NUM_THREADS] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
pthread_mutex_t mutex[NUM_THREADS];

void *routine(void *arg) {
    long int index = (long int)arg;

    pthread_mutex_lock(mutex + index);
    printf("%d ", primes[index]);
    pthread_mutex_unlock(mutex + ((index + 1) % NUM_THREADS));

    pthread_exit(NULL);
}

int main(void) {
    pthread_t tid[NUM_THREADS];
    long int i;

    for (i = 0; i < NUM_THREADS; i++) {
        if (pthread_mutex_init(mutex + i, NULL) || pthread_mutex_lock(mutex + i)) {
            perror("pthread_mutex_init or pthread_mutex_lock");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(tid + i, NULL, &routine, (void *)i) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    pthread_mutex_unlock(mutex);

    for (i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(tid[i], NULL) || pthread_mutex_destroy(mutex + i)) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    puts("");
    return 0;
}
