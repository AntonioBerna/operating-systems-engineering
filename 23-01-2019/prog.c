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
#include <assert.h>

char *filename;
int no_threads;
char buffer[BUFSIZ];

typedef struct {
	int thread_no;
	char *string;
} thread_args_t;

typedef thread_args_t *thread_args_ptr_t;

typedef int semaphore;
semaphore sem_go_ahead, sem_is_ready;

void *search_string(void *arg) {
	thread_args_ptr_t my_arg = (thread_args_ptr_t)arg;
	int thread_no = my_arg->thread_no;
	char *string = my_arg->string;
	free(my_arg);

#ifdef DEBUG
	printf("Thread no. %d handles the string %s\n", thread_no, string);
#endif

	struct sembuf sops;

	sigset_t set;
	sigfillset(&set);
	if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
		perror("sigprocmask");
		exit(EXIT_FAILURE);
	}

	while (1) {
		sops.sem_num = thread_no;
		sops.sem_op = -1;
		if (semop(sem_is_ready, &sops, 1) == -1) {
			perror("semop3");
			exit(EXIT_FAILURE);
		}
		
		// start of critical section

		if (strcmp(buffer, string) == 0) {
#ifdef DEBUG
			printf("Thread no. %d matched the string %s\n", thread_no, string);
#endif
			strncpy(buffer, "*", sizeof(short));
			for (size_t i = 0; i < strlen(string) - 1; ++i) strncat(buffer, "*", sizeof(short));
		}
		
		// end of critical section

		sops.sem_num = thread_no;
		sops.sem_op = 1;
		if (semop(sem_go_ahead, &sops, 1) == -1) {
			perror("semop");
			exit(EXIT_FAILURE);
		}
	}
	
	return NULL;
}

void file_printer(int sig_no) {
	(void)sig_no;
	
	char command[BUFSIZ];
	snprintf(command, sizeof(command), "cat %s", filename);
	
	puts("--- OVERALL OUTPUT ---");
	system(command);
	puts("--- OUTPUT PRINTED ---");
}

void free_all_resources(int sig_no) {
	(void)sig_no;
	
	for (int i = 0; i < no_threads; ++i) {
		semctl(sem_go_ahead, i, IPC_RMID);
		semctl(sem_is_ready, i, IPC_RMID);
	}

#ifdef DEBUG
	puts("The free of all resources has been completed.");
	puts("Check this using \"ipcs -a\" command.");
#endif

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage: %s <filename> <str-1> ... <str-N>\n", *argv);
		exit(EXIT_FAILURE);
	}

	filename = argv[1];
	no_threads = argc - 2;
	
	int fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0660);
	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	strncpy(buffer, "\0", sizeof(char));

	sem_go_ahead = semget(IPC_PRIVATE, no_threads, IPC_CREAT | IPC_EXCL | 0660);
	if (sem_go_ahead == -1) {
		perror("semget");
		exit(EXIT_FAILURE);
	}

	sem_is_ready = semget(IPC_PRIVATE, no_threads, IPC_CREAT | IPC_EXCL | 0660);
	if (sem_is_ready == -1) {
		perror("semget");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i < no_threads; ++i) {
		if (semctl(sem_go_ahead, i, SETVAL, 1) == -1) { 
			perror("semctl");
			exit(EXIT_FAILURE);
		}

		if (semctl(sem_is_ready, i, SETVAL, 0) == -1) {
			perror("semctl");
			exit(EXIT_FAILURE);
		}
	}

	signal(SIGINT, file_printer);
	signal(SIGQUIT, free_all_resources);

	pthread_t new_thread;
	for (int i = 0; i < no_threads; ++i) {
		thread_args_ptr_t new_arg = calloc(1, sizeof(thread_args_t));
		assert(new_arg != NULL && "Buy More RAM lol");
		new_arg->thread_no = i;
		new_arg->string = argv[i + 2];
		
		if (pthread_create(&new_thread, NULL, &search_string, new_arg) != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}

	struct sembuf sops;

	while (1) {
		for (int i = 0; i < no_threads; ++i) {
try_again_wait_op:
			sops.sem_num = i;
			sops.sem_op = -1;
			if (semop(sem_go_ahead, &sops, 1) == -1) {
				if (errno == EINTR) {
					goto try_again_wait_op;
				}
				perror("semop");
				exit(EXIT_FAILURE);
			}
		}
		
		// start of critical section

		if (strcmp(buffer, "\0") != 0) {
			if (dprintf(fd, "%s\n", buffer) < 0) {
				perror("dprintf");
				exit(EXIT_FAILURE);
			}
		}

try_again_fgets:
		if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
			buffer[strcspn(buffer, "\n")] = '\0';
		}

		if (strlen(buffer) >= sizeof(buffer)) {
			fprintf(stderr, "The string you provided is too large! Please provide a shorter one.\n");
			goto try_again_fgets;
		}
	
		// end of critical section
		
		for (int i = 0; i < no_threads; ++i) {
try_again_post_op:
			sops.sem_num = i;
			sops.sem_op = 1;
			if (semop(sem_is_ready, &sops, 1) == -1) {
				if (errno == EINTR) {
					goto try_again_post_op;
				}
				perror("semop");
				exit(EXIT_FAILURE);
			}
		}
	}
	
	return 0;
}

