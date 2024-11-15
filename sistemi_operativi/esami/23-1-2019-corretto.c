#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/sem.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>

#define LINE_SIZE 1024

typedef struct param_thread{
	int me;
	char *my_string;} param;

char buf[LINE_SIZE] = {0};
int sem_written, sem_read;
int file_descriptor;
int n;


void handler(int sign){
	printf("Received SIGINT - I'm printing file\n\n");
	fflush(stdout);
	system("cat F.txt\n");
}


void *comparator(void *p){
	param *args = (param *)p;
	struct sembuf sops[1];
	sops[0].sem_num = 0;
	sops[0].sem_flg = 0;
	while(1){
		sops[0].sem_op = -1;
	semaph_written:
		errno = 0;
		if(semop(sem_written, sops, 1) == -1){
			if(errno == EINTR){
				goto semaph_written;}
			else{
				printf("Thread %d couldn't take token from sem_written\n", args->me);
				fflush(stdout);
				printf("Errno is %d\n\n", errno);
				exit(EXIT_FAILURE);}
		}

		if(strcmp(args->my_string, buf) == 0){
			for(int i = 0; i < strlen(args->my_string); i++){
				buf[i] = '*';}
		}
	
		sops[0].sem_op = 1;	
	semaph_read:
		errno = 0;	
		if(semop(sem_read, sops, 1) == -1){
			if(errno == EINTR) goto semaph_read;
			else{
				printf("Thread %d couldn't put token into sem_read\n\n", args->me);
				fflush(stdout);
				exit(EXIT_FAILURE);}
		}
	}
}

int main(int argc, char**argv){
	
	if(argc < 2){
		printf("Usage: prog <string1> <string2> ...\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	
	n = argc - 1;
	struct sembuf sops[1];
	pthread_t tid;

	sem_written = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
	if(sem_written == -1){
		printf("Sem_written creation error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	if(semctl(sem_written, 0, SETVAL, 0) == -1){
		printf("Sem_written initialisation error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	sem_read = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
	if(sem_read == -1){
		printf("Sem_read creation error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	if(semctl(sem_read, 0, SETVAL, 0) == -1){
		printf("Sem_read initialisation error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}


	file_descriptor = open("F.txt", O_CREAT | O_TRUNC | O_RDWR, 0777);
	if(file_descriptor == -1){
		printf("File opening error\n\n");
		fflush(stdout);
		printf("Errno is %d\n\n", errno);}
	
	if(signal(SIGINT, handler) == SIG_ERR){
		printf("Signal error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}


	for(int i = 0; i < n; i++){
		param *p = (param *)malloc(sizeof(param *));
		p->me = i;
		p->my_string = argv[i+1];
		if(pthread_create(&tid, NULL, comparator, (void *)p) == -1){
			printf("Couldn't create thread %d\n\n", i);
			fflush(stdout);
			exit(EXIT_FAILURE);}
	}
		
	sops[0].sem_num = 0;
	sops[0].sem_flg = 0;
	while(1){

read_from_stdin:
		errno = 0;
		if(fgets(buf, LINE_SIZE - 1, stdin) == 0){
			if(errno == EINTR){
				goto read_from_stdin;}
			else{
				printf("Couldn't get string form stdin\n\n");
				fflush(stdin);
				exit(EXIT_FAILURE);}
		}
		buf[strlen(buf)-1] = '\0';

		sops[0].sem_op = n;
sem_written:
		errno = 0;
		if(semop(sem_written, sops, 1) == -1){
			if(errno == EINTR){
				goto sem_written;}
			else{
				printf("Main thread couldn't put %d tokens in semaphore\n", n);
				fflush(stdout);
				exit(EXIT_FAILURE);}
		}

		sops[0].sem_op = -n;
sem_wr:
                errno = 0;
                if(semop(sem_read, sops, 1) == -1){
                        if(errno == EINTR) goto sem_wr;
                        else{
                                printf("Main thread couldn't get %d tokens from semaphore\n", n);
                                fflush(stdout);
                                printf("Errno is %d\n\n", errno);
                                fflush(stdout);
                                exit(EXIT_FAILURE);}
                }

		int prev_bytes_written = 0, bytes_written = 0, remaining_bytes = strlen(buf);
file_wr:
                errno = 0;
                bytes_written = write(file_descriptor, &buf[prev_bytes_written], remaining_bytes);
                if(bytes_written == -1 && errno == EINTR) goto file_wr;
                else if(bytes_written == -1){
                        printf("Couldn't write string on file F\n\n");
                        fflush(stdout);
                        exit(EXIT_FAILURE);}
                prev_bytes_written = prev_bytes_written + bytes_written;
                if(prev_bytes_written != remaining_bytes){
                        goto file_wr;}
                write(file_descriptor, "\n", 1);

	}
	
}
