#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <signal.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef int semaphore;
semaphore semid;

typedef int shared_memory;
shared_memory shmid;
char *shm_ptr;

void my_free(int sig_no) {
    (void)sig_no;

    semctl(semid, 0, IPC_RMID);
	semctl(semid, 1, IPC_RMID);

    shmdt(shm_ptr);
    shmctl(shmid, IPC_RMID, NULL);

    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <N>\n", *argv);
        exit(EXIT_FAILURE);
    }

    int N = atoi(argv[1]);

    semid = semget(IPC_PRIVATE, 2, IPC_CREAT | IPC_EXCL | 0660);
    if (semid == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    semctl(semid, 0, SETVAL, 1);
    semctl(semid, 1, SETVAL, 0);

    shmid = shmget(IPC_PRIVATE, BUFSIZ, IPC_CREAT | IPC_EXCL | 0660);
    if (shmid == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    shm_ptr = (char *)shmat(shmid, NULL, 0);
    if (shm_ptr == (char *)(-1)) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
    
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = &my_free;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    strncpy(shm_ptr, "ping", 5);

    struct sembuf sops;

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // child process

        for (int i = 0; i < N; ++i) {
            sops.sem_num = 0;
            sops.sem_op = -1;
            semop(semid, &sops, 1);

            shm_ptr[1] = 'i';
            puts(shm_ptr);

            sops.sem_num = 1;
            sops.sem_op = 1;
            semop(semid, &sops, 1);
        }
    
    } else { // parent process

        for (int i = 0; i < N; ++i) {
            sops.sem_num = 1;
            sops.sem_op = -1;
            semop(semid, &sops, 1);
            
            shm_ptr[1] = 'o';
            puts(shm_ptr);

            sops.sem_num = 0;
            sops.sem_op = 1;
            semop(semid, &sops, 1);
        }

    }

    pause();

    return 0;
}
