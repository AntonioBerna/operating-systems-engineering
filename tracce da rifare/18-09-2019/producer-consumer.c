#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

#define THREAD_NUM 2

sem_t sem_empty;
sem_t sem_full;

pthread_mutex_t mutex;
int buffer[10];
int count = 0;

_Noreturn void *producer(void *arg) {
    (void)arg;
    while (1) {
        int x = rand() % 100;
        sleep(1);

        sem_wait(&sem_empty);
        pthread_mutex_lock(&mutex);
        if (count < 10) {
            buffer[count++] = x;
        } else {
            printf("Skipped %d\n", x);
        }
        pthread_mutex_unlock(&mutex);
        sem_post(&sem_full);
    }
}

_Noreturn void *consumer(void *arg) {
    (void)arg;
    while (1) {
        int y = -1;
        sem_wait(&sem_full);
        pthread_mutex_lock(&mutex);
        if (count > 0) {
            y = buffer[(count--) - 1];
        }
        pthread_mutex_unlock(&mutex);
        sem_post(&sem_empty);

        printf("Got %d\n", y);
        sleep(1);
    }
}

int main(void) {
    srand(time(NULL));

    pthread_t tid[THREAD_NUM];

    pthread_mutex_init(&mutex, NULL);
    sem_init(&sem_empty, 0, 10);
    sem_init(&sem_full, 0, 0);

    int i;

    for (i = 0; i < THREAD_NUM; i++) {
        if (i % 2 == 0) {
            if (pthread_create(tid + i, NULL, &producer, NULL) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        } else {
            if (pthread_create(tid + i, NULL, &consumer, NULL) != 0) {
                perror("pthread_create");
                exit(EXIT_FAILURE);
            }
        }
    }

    for (i = 0; i < THREAD_NUM; i++) {
        if (pthread_join(tid[i], NULL) != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }
    }

    sem_destroy(&sem_empty);
    sem_destroy(&sem_full);
    pthread_mutex_destroy(&mutex);

    return 0;
}