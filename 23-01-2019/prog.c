#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	int thread_no;
	char *buffer;
	char *str;
} thread_args_t;

typedef thread_args_t *thread_args_ptr_t;

void *cmp_and_replace(void *arg) {
	thread_args_ptr_t thread_args = (thread_args_ptr_t)arg;
	int thread_no = thread_args->thread_no;
	free(thread_args);
	
	int sem_go_ahead = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
	if (sem_go_ahead == -1) {
		perror("semget");
		exit(EXIT_FAILURE);
	}

	int sem_is_ready = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
	if (sem_is_ready == -1) {
		perror("semget");
		exit(EXIT_FAILURE);
	}

	struct sembuf sops;
	sops.sem_num = 0;
	sops.sem_op = -1;
	semop(sem_is_ready, &sops, 1);

	if (strcmp(input_buffer, 	
	

	return NULL;
}

int main(int argc, char **argv) {
	if (argc < 3) {
		printf("Usage: %s <filename> <str-1> ... <str-N>\n", *argv);
		exit(EXIT_FAILURE);
	}
	
	int sem_go_ahead = semget(IPC_PRIVATE, argc - 2, 0666 | IPC_CREAT);
	if (sem_go_ahead == -1) {
		perror("semget");
		exit(EXIT_FAILURE);
	}

	int sem_is_ready = semget(IPC_PRIVATE, argc - 2, 0666 | IPC_CREAT);
	if (sem_is_ready == -1) {
		perror("semget");
		exit(EXIT_FAILURE);
	}

	pthread_t new_thread;
	for (size_t idx = 1; idx < argc - 1; ++idx) {
		if (semctl(sem_go_ahead, idx - 1, SETVAL, 1) == -1) {
			perror("semctl");
			exit(EXIT_FAILURE);
		}

		if (semctl(sem_is_ready, idx - 1, SETVAL, 0) == -1) {
			perror("semctl");
			exit(EXIT_FAILURE);
		}

		thread_args_ptr_t thread_args = malloc(sizeof(thread_args_t));
		if (thread_args == NULL) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		thread_args->thread_no = (int)(idx - 1);
		thread_args->str = argv[idx];

		if (pthread_create(&new_thread, NULL, cmp_and_replace, thread_args) != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}

	char input_buffer[BUFSIZ];
	input_buffer[BUFSIZ - 1] = '\0';

	struct sembuf sops;
	
	while (1) {
		sops.sem_num = (int)cycle_index;
		sops.sem_op = -1;
		semop(sem_go_ahead, &sops, 1);

		if (strcmp(input_buffer, "\0") != 0) {
			fprintf(fp, "%s\n", input_buffer);
			fflush(fp);
		}

		if (fgets(input_buffer, sizeof(input_buffer) - 1, stdin) == NULL) {
			perror("fgets");
			exit(EXIT_FAILURE);
		}

		if (strlen(input_buffer) >= BUFSIZ - 1) {
			fprintf(stderr, "The string you provided is too large! Please provide a shorter one.\n");
			continue;
		}

		sops.sem_num = (int)cycle_index;
		sops.sem_op = 1;
		semop(sem_is_ready, &sops, 1);

		cycle_index = (cycle_index + 1) % (argc - 2);
	}

	return 0;
}

