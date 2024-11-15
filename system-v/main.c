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

typedef struct {
	int thread_no;
	int fd;
} thread_args_t;

typedef thread_args_t *thread_args_ptr_t;

typedef int semaphore;
semaphore sem_thread;
semaphore sem_main;

int *input_file_array;
char **input_string_array;
int no_files;

void *thread_routine(void *arg) {
	thread_args_ptr_t my_arg = (thread_args_ptr_t)arg;
	int thread_no = my_arg->thread_no;
	int fd = my_arg->fd;
	free(my_arg);

	struct sembuf sops;

#ifdef DEBUG
	printf("Thread no. %d opens file descriptor %d\n", thread_no, fd);
	fflush(stdout);
#endif

	// sigset_t set;
	// sigfillset(&set);
	// if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
	// 	perror("sigprocmask");
	// 	exit(EXIT_FAILURE);
	// }
	sigset_t set;
	sigfillset(&set);
	if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0) {
		perror("pthread_sigmask");
		exit(EXIT_FAILURE);
	}

	while (1) {
		sops.sem_num = thread_no;
		sops.sem_op = -1;

		if (semop(sem_main, &sops, 1) == -1) {
			perror("semop");
			exit(EXIT_FAILURE);
		}
		
		if (dprintf(fd, "%s", *(input_string_array + thread_no)) < 0) {
			perror("dprintf");
			exit(EXIT_FAILURE);
		}

#ifdef DEBUG
		int len = (int)strlen(*(input_string_array + thread_no)) - 1;
		printf("String %.*s printed correctly onto file by thread no. %d\n", len, *(input_string_array + thread_no), thread_no);
#endif
		
		sops.sem_num = thread_no;
		sops.sem_op = 1;

		if (semop(sem_thread, &sops, 1) == -1) {
			perror("semop");
			exit(EXIT_FAILURE);
		}
	}

	return NULL;
}

void signal_routine(int sig_no) {
	(void)sig_no;

	lseek(*input_file_array, 0, SEEK_SET);

	FILE *file_stream;
	if ((file_stream = fdopen(*input_file_array, "r+")) == NULL) {
		perror("fdopen");
		exit(EXIT_FAILURE);
	}

	char *filename = "output.txt";
	int output_fd;
	if ((output_fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0660)) == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	char new_string[BUFSIZ];
	int no_fails = 0;
	while (1) {
		errno = 0;
		if (fgets(new_string, BUFSIZ, file_stream) == NULL) {
			if (errno != 0) {
				perror("fgets");
				exit(EXIT_FAILURE);
			}
			no_fails++;
		} else {
			dprintf(output_fd, "%s", new_string);
		}

		if (no_fails == no_files) break;
		no_fails = 0;
	}

	close(output_fd);
	
	puts("--- OVERALL OUTPUT ---");
	char command[BUFSIZ];
	sprintf(command, "cat %s", filename);
	system(command);
	puts("--- OUTPUT PRINTED ---");
	fflush(stdout);
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <filename>\n", *argv);
		exit(EXIT_FAILURE);
	}
	
	input_file_array = calloc(argc - 1, sizeof(*input_file_array));
	if (input_file_array == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	no_files = argc - 1;

	input_string_array = calloc(argc - 1, sizeof(*input_string_array));
	if (input_string_array == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}
		
	if ((sem_thread = semget(IPC_PRIVATE, argc - 1, 0660)) == -1) {
		perror("semget");
		exit(EXIT_FAILURE);
	}

	if ((sem_main = semget(IPC_PRIVATE, argc - 1, 0660)) == -1) {
		perror("semget");
		exit(EXIT_FAILURE);
	}

    // signal(SIGINT, &signal_routine);
	struct sigaction act;
	act.sa_handler = signal_routine;
	if (sigaction(SIGINT, &act, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	if (semctl(sem_thread, 0, SETVAL, 1) == -1) {
		perror("semctl");
		exit(EXIT_FAILURE);
	}

	if (semctl(sem_main, 0, SETVAL, 0) == -1) {
		perror("semctl");
		exit(EXIT_FAILURE);
	}
	
	thread_args_ptr_t new_arg = calloc(1, sizeof(thread_args_t));
	if (new_arg == NULL) {
		perror("calloc");
		exit(EXIT_FAILURE);
	}

	new_arg->thread_no = 0;
	if ((new_arg->fd = open(*(argv + 1), O_CREAT | O_RDWR | O_TRUNC, 0660)) == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	input_file_array[0] = new_arg->fd;
	
	pthread_t new_thread;
	if (pthread_create(&new_thread, NULL, &thread_routine, new_arg) != 0) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}

	char input_buffer[BUFSIZ];
	input_buffer[BUFSIZ - 1] = '\0';

	struct sembuf sops;
	
	size_t cycle_index = 0;

	while (1) {
#ifdef DEBUG
		printf("Cycle no. %d\n", (int)cycle_index);
#endif

try_again_read:
		if (fgets(input_buffer, BUFSIZ - 1, stdin) == NULL) {
			if (errno == EINTR) goto try_again_read;
			perror("fgets");
			exit(EXIT_FAILURE);
		}
		
		if (strlen(input_buffer) >= BUFSIZ - 1) {
			fprintf(stderr, "The string you provided is too large! Please provide a shorter one.\n");
			continue;
		}

		sops.sem_num = (int)cycle_index;
		sops.sem_op = -1;

try_again_op1:
		if (semop(sem_thread, &sops, 1) == -1) {
			if (errno == EINTR) goto try_again_op1;
			perror("semop");
			exit(EXIT_FAILURE);
		}
		
		if (*(input_string_array + cycle_index) != NULL) {
			free(*(input_string_array + cycle_index));
		}

		if ((*(input_string_array + cycle_index) = strdup(input_buffer)) == NULL) {
			perror("strdup");
			exit(EXIT_FAILURE);
		}
		
		sops.sem_num = (int)cycle_index;
		sops.sem_op = 1;
		
try_again_op2:
		if (semop(sem_main, &sops, 1) == -1) {
			if (errno == EINTR) goto try_again_op2;
			perror("semop");
			exit(EXIT_FAILURE);
		}

		cycle_index = (cycle_index + 1) % (argc - 1);
	}

	return 0;
}
