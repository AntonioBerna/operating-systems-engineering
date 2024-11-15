#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>

typedef struct {
	int thread_no;
	int N;
} thread_args_t;

void *ping_routine(void *arg) {
	thread_args_t *args = (thread_args_t *)arg;
	int N = args->N;
	int id = args->thread_no;
	free(args);
	
	struct sembuf sops;

    for (size_t i = 0; i < N; ++i) {
        sops.sem_num = 0;
        sops.sem_op = -1;
        semop(id, &sops, 1);

        printf("ping\n");

        sops.sem_num = 1;
        sops.sem_op = 1;
        semop(id, &sops, 1);
    }

	return NULL;
}

void *pong_routine(void *arg) {
	thread_args_t *args = (thread_args_t *)arg;
	int N = args->N;
	int id = args->thread_no;
	free(args);

	struct sembuf sops;
    
	for (size_t i = 0; i < N; ++i) {
		sops.sem_num = 1;
        sops.sem_op = -1;
        semop(id, &sops, 1);

        printf("pong\n");

        sops.sem_num = 0;
        sops.sem_op = 1;
        semop(id, &sops, 1);
	}

	return NULL;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <N>\n", *argv);
		exit(EXIT_FAILURE);
	}

    long id = semget(IPC_PRIVATE, 2, 0666 | IPC_CREAT);
    if (id == -1) {
		perror("semget");
        exit(EXIT_FAILURE);
    }
	
    semctl(id, 0, SETVAL, 1);
    semctl(id, 1, SETVAL, 0);
	
	thread_args_t *ping_args = malloc(sizeof(thread_args_t));
	ping_args->thread_no = id;
	ping_args->N = atoi(argv[1]);

	thread_args_t *pong_args = malloc(sizeof(thread_args_t));
	pong_args->thread_no = id;
	pong_args->N = atoi(argv[1]);

	pthread_t ping_t, pong_t;
    pthread_create(&ping_t, NULL, &ping_routine, ping_args);
    pthread_create(&pong_t, NULL, &pong_routine, pong_args);
	
	pthread_join(ping_t, NULL);
   	pthread_join(pong_t, NULL);
	
	free(ping_args);
	free(pong_args);

   	semctl(id, 0, IPC_RMID);
	semctl(id, 1, IPC_RMID);

   	return 0;
}
