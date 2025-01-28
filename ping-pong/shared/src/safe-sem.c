#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "safe-sem.h"

int safe_sem_init(safe_sem_t *sem, int key, int sem_num, int init_value) {
    sem->semid = semget(key, 1, 0644 | IPC_CREAT);
    if (sem->semid == -1) {
        perror("semget");
        return -1;
    }

    sem->sem_num = sem_num;

    union semun {
        int val;
        struct semid_ds *buf;
        unsigned short *array;
    } arg;
    arg.val = init_value;

    if (semctl(sem->semid, sem_num, SETVAL, arg) == -1) {
        perror("semctl SETVAL");
        return -1;
    }

    return 0;
}

int safe_sem_wait(safe_sem_t *sem) {
    struct sembuf sop;
    sop.sem_num = sem->sem_num;
    sop.sem_op = -1;
    sop.sem_flg = 0;

    if (semop(sem->semid, &sop, 1) == -1) {
        perror("semop wait");
        return -1;
    }

    return 0;
}

int safe_sem_post(safe_sem_t *sem) {
    struct sembuf sop;
    sop.sem_num = sem->sem_num;
    sop.sem_op = 1;
    sop.sem_flg = 0;

    if (semop(sem->semid, &sop, 1) == -1) {
        perror("semop post");
        return -1;
    }

    return 0;
}

int safe_sem_destroy(safe_sem_t *sem) {
    if (semctl(sem->semid, 0, IPC_RMID) == -1) {
        perror("semctl IPC_RMID");
        return -1;
    }

    return 0;
}
