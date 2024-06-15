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
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;

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
	printf("Thread no. %d handles the string \"%s\"\n", thread_no, string);
#endif

	struct sembuf sops;

	while (1) {
		sops.sem_num = thread_no;
		sops.sem_op = -1;
		while (semop(sem_is_ready, &sops, 1) == -1) {
			if (errno == EINTR) continue;
			if (errno == EIDRM) pthread_exit(NULL);
			perror("semop");
			exit(EXIT_FAILURE);
		}
		
		// start of critical section
		pthread_mutex_lock(&buffer_mutex);

		if (strcmp(buffer, string) == 0) {
#ifdef DEBUG
			printf("Thread no. %d matched the string \"%s\"\n", thread_no, string);
#endif
			memset(buffer, '*', strlen(buffer));
		}
		
		pthread_mutex_unlock(&buffer_mutex);
		// end of critical section

		sops.sem_num = thread_no;
		sops.sem_op = 1;
		while (semop(sem_go_ahead, &sops, 1) == -1) {
            if (errno == EINTR) continue;
            if (errno == EIDRM) pthread_exit(NULL);
            perror("semop");
            exit(EXIT_FAILURE);
        }
	}
	
	return NULL;
}

void file_printer(int sig_no) {
    (void)sig_no;

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("fopen");
        return;
    }

    puts("--- OVERALL OUTPUT ---");
    char line[BUFSIZ];
    while (fgets(line, sizeof(line), file) != NULL) {
        fputs(line, stdout);
    }
    puts("--- OUTPUT PRINTED ---");

    if (fclose(file) == EOF) {
        perror("fclose");
    }
}

void free_all_resources(int sig_no) {
	(void)sig_no;
	
	for (int i = 0; i < no_threads; ++i) {
		semctl(sem_go_ahead, i, IPC_RMID, 0);
		semctl(sem_is_ready, i, IPC_RMID, 0);
	}

#ifdef DEBUG
	puts("The free of all resources has been completed.");
	puts("Check this using \"ipcs -s\" command.");
#endif

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "Usage: %s [filename] [str-1] ... [str-N]\n", *argv);
		exit(EXIT_FAILURE);
	}

	filename = argv[1];
	no_threads = argc - 2;
	
	int fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0660);
	if (fd == -1) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	memset(buffer, 0, sizeof(buffer));

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

#if 0
	if (signal(SIGINT, file_printer) == SIG_ERR) {
		perror("signal");
		exit(EXIT_FAILURE);
	}

	if (signal(SIGQUIT, free_all_resources) == SIG_ERR) {
		perror("signal");
		exit(EXIT_FAILURE);
	}
#else
	struct sigaction sa;
	sa.sa_handler = &file_printer;
	sa.sa_flags = SA_RESTART;
	sigfillset(&sa.sa_mask);

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	sa.sa_handler = &free_all_resources;

	if (sigaction(SIGQUIT, &sa, NULL) == -1) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}
#endif

	pthread_t new_thread;
	for (int i = 0; i < no_threads; ++i) {
		thread_args_ptr_t new_arg = calloc(1, sizeof(thread_args_t));
		if (new_arg == NULL) {
			perror("calloc");
			exit(EXIT_FAILURE);
		}
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
			sops.sem_num = i;
			sops.sem_op = -1;
			while (semop(sem_go_ahead, &sops, 1) == -1) {
                if (errno == EINTR) continue;
                perror("semop");
                exit(EXIT_FAILURE);
            }
		}
		
		// start of critical section
		pthread_mutex_lock(&buffer_mutex);

		if (strcmp(buffer, "\0") != 0) {
			if (dprintf(fd, "%s\n", buffer) < 0) {
				perror("dprintf");
				close(fd);
				exit(EXIT_FAILURE);
			}
			memset(buffer, 0, sizeof(buffer));
		}
		
		while (fgets(buffer, sizeof(buffer), stdin) == NULL) {
            if (errno == EINTR) continue;
            perror("fgets");
            exit(EXIT_FAILURE);
        }
		buffer[strcspn(buffer, "\n")] = '\0';

		if (strlen(buffer) >= sizeof(buffer)) {
			fprintf(stderr, "The string you provided is too large! Please provide a shorter one.\n");
			continue;
		}

		pthread_mutex_unlock(&buffer_mutex);
		// end of critical section
		
		for (int i = 0; i < no_threads; ++i) {
			sops.sem_num = i;
			sops.sem_op = 1;
			while (semop(sem_is_ready, &sops, 1) == -1) {
                if (errno == EINTR) continue;
                perror("semop");
                exit(EXIT_FAILURE);
            }
		}
	}
	
	return 0;
}

