// Leggi N stringhe da riga di comando e crea N thread, ognuno dei quali stampa la stringa ricevuta al contrario.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int sem_id;

void swap(char *x, char *y) {
	char t = *x;
	*x = *y;
	*y = t;
}

void *print_inverse(void *arg) {
    char *str = (char *)arg;
	
	struct sembuf sops;
	
	sops.sem_num = 0;
	sops.sem_op = -1;
	semop(sem_id, &sops, 1);

	// start of critical section
	
	for (size_t i = 0, j = strlen(str) - 1; i < j; ++i, --j) swap(&str[i], &str[j]);
    printf("%s\n", str);
	
	// end of critical section
	
	sops.sem_num = 0;
	sops.sem_op = 1;
	semop(sem_id, &sops, 1);
	
	pthread_exit(NULL);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <str-1> <str-2> ... <str-N>\n", *argv);
        exit(EXIT_FAILURE);
    }

    sem_id = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
    semctl(sem_id, 0, SETVAL, 1);

    pthread_t threads[argc - 1];
    for (int i = 0; i < argc - 1; ++i) {
        if (pthread_create(&threads[i], NULL, &print_inverse, argv[i + 1]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < argc - 1; ++i) {
        pthread_join(threads[i], NULL);
    }

    semctl(sem_id, 0, IPC_RMID);

    return 0;
}
