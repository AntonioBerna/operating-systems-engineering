#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define THREAD_NUM 4

typedef struct Task {
    int a, b;
} Task;

Task task_queue[256];
int task_count = 0;

pthread_mutex_t mutex_queue;
pthread_cond_t cond_queue;

void execute_task(Task *task) {
    usleep(50000);
    int result = task->a + task->b;
    printf("The sum of %d and %d is %d\n", task->a, task->b, result);
}

void submit_task(Task task) {
    pthread_mutex_lock(&mutex_queue);
    task_queue[task_count++] = task;
    pthread_mutex_unlock(&mutex_queue);
    pthread_cond_signal(&cond_queue);
}

_Noreturn void *start_thread(void *arg) {
    (void)arg;
    while (1) {
        Task task;

        pthread_mutex_lock(&mutex_queue);
        while (task_count == 0) {
            pthread_cond_wait(&cond_queue, &mutex_queue);
        }
        task = task_queue[0];
        for (int i = 0; i < task_count - 1; i++) {
            task_queue[i] = task_queue[i + 1];
        }
        task_count--;
        pthread_mutex_unlock(&mutex_queue);
        execute_task(&task);
    }
}

int main(void) {
    pthread_t th[THREAD_NUM];
    pthread_mutex_init(&mutex_queue, NULL);
    pthread_cond_init(&cond_queue, NULL);
    int i;

    for (i = 0; i < THREAD_NUM; i++) {
        if (pthread_create(th + i, NULL, &start_thread, NULL) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    srand(time(NULL));
    for (i = 0; i < 100; i++) {
        Task t = { .a = rand() % 100, .b = rand() % 100 };
        submit_task(t);
    }

    for (i = 0; i < THREAD_NUM; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("pthread_join");
            exit(EXIT_FAILURE);
        }

    }

    pthread_mutex_destroy(&mutex_queue);
    pthread_cond_destroy(&cond_queue);

    return 0;
}