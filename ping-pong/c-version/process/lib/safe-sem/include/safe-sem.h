#pragma once

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

typedef struct {
    int semid;
    int sem_num;
} safe_sem_t;

int safe_sem_init(safe_sem_t *sem, int key, int sem_num, int init_value);
int safe_sem_wait(safe_sem_t *sem);
int safe_sem_post(safe_sem_t *sem);
int safe_sem_destroy(safe_sem_t *sem);
