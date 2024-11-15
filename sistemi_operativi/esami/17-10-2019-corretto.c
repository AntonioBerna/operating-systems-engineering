#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/syscall.h>

#define LINE_SIZE 1024

int num_threads;
FILE **fd;
char **buf;
int sem_written, sem_read;

void *thread(void *p){
	int me = *((int *)p);
	struct sembuf sops[1];
	sops[0].sem_flg = 0;
	sops[0].sem_num = me;
	
	while(1){
		sops[0].sem_op = -1;
thread_semwritten_wait:
		errno = 0;
		if(semop(sem_written, sops, 1) == -1){
		        if(errno == EINTR) goto thread_semwritten_wait;
			else{
				printf("Thread %d couldn't take token from sem_written\n\n", me);
				fflush(stdout);
				exit(EXIT_FAILURE);}
		}
		
		printf("Thread %d received string %s", me, buf[me]);
		printf("Thread %d is writing on file\n\n", me);
		fflush(stdout);
	
write_on_file:
		errno = 0;
		if(fprintf(fd[me], "%s", buf[me]) == 0){ 
			if(errno == EINTR) goto write_on_file;
			else{
				printf("Couldn't wrtie on file\n\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
		}		

		sops[0].sem_op = 1;
thread_semread_signal:
		errno = 0;
		if(semop(sem_read, sops, 1) == -1){
		       if(errno == EINTR) goto thread_semread_signal;
			else{
				printf("Thread %d couldn't put token into sem_read\n\n", me);
				fflush(stdout);	
				exit(EXIT_FAILURE);}
		}
	}
}



void handler(int sign){
	printf("Received SIGINT\n\n");
	fflush(stdout);
	sigset_t set;
	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, NULL);
	char buff[LINE_SIZE];
	FILE *file_desc = fopen("output-file.txt", "w+");
	if(file_desc == NULL){
		printf("Couldn't create output file\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	for(int i = 0; i<num_threads; i++){
		fseek(fd[i], 0, SEEK_SET);}
	int i = 0;
	while(1){
			if(fgets(buff, 1024, fd[i]) == 0){
			        printf("Output file written\n\n");	
				break;}
			fprintf(file_desc, "%s", buff);
			
			i = (i+1)%num_threads;
	}
	fclose(file_desc);
	system("cat output-file.txt\n");
	sigprocmask(SIG_UNBLOCK, &set, NULL);
}


int main(int argc, char **argv){
	if(argc < 2){
		printf("Usage: prog <filename1> <filename2> ...\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	pthread_t tid;
	struct sigaction sa;
	sigset_t set;

	num_threads = argc - 1;
	
	fd = (FILE **)malloc(sizeof(FILE *) * num_threads);
	if(fd == NULL){
		printf("Malloc fd error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	for(int i = 0; i<num_threads; i++){
		fd[i] = fopen(argv[i+1], "w+");
		if(fd[i] == NULL){	
			printf("file %d open error\n\n", i);
			fflush(stdout);
			exit(EXIT_FAILURE);}
	}


	buf = (char **)malloc(sizeof(char **) * num_threads);
	if(buf == NULL){
		printf("Buf malloc error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	for(int i = 0; i<num_threads; i++){
		buf[i] = (char *)malloc(sizeof(char) * LINE_SIZE);
		if(buf[i] == NULL){
			printf("Buf[%d] malloc error\n\n", i);
			fflush(stdout);
			exit(EXIT_FAILURE);}
	}


	sem_written = semget(IPC_PRIVATE, num_threads, IPC_PRIVATE | 0666);
	if(sem_written == -1){
		printf("Sem_written creation error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	for(int i = 0; i<num_threads; i++){
		if(semctl(sem_written, i, SETVAL, 0) == -1){
			printf("Sem_written initialization error\n\n");
			fflush(stdout);
			exit(EXIT_FAILURE);}
	}


	sem_read = semget(IPC_PRIVATE, num_threads, IPC_CREAT | 0666);
	if(sem_read == -1){
		printf("Sem_readn creation error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	for(int i = 0; i<num_threads; i++){
		if(semctl(sem_read, i, SETVAL, 1) == -1){
			printf("Sem_read initialization error\n\n");
			fflush(stdout);
			exit(EXIT_FAILURE);}
	}
	

	for(int i = 0; i<num_threads; i++){
		int *p = (int *)malloc(sizeof(int));
		if(p == NULL){
			printf("param[%d] malloc error\n\n", i);
			fflush(stdout);
			exit(EXIT_FAILURE);}
		*p = i;
		if(pthread_create(&tid, NULL, thread, (void *)p) == -1){
			printf("Thread %d creation error\n\n", i);
			fflush(stdout);
			exit(EXIT_FAILURE);}
		}


	memset(&sa, 0, sizeof(sa));
	sigfillset(&set);
	sa.sa_handler = handler;
	sa.sa_mask = set;
	if(sigaction(SIGINT, &sa, NULL) == -1){
		printf("Sigaction error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	struct sembuf sops[1];
	sops[0].sem_flg = 0;
	
	
	while(1){
		for(int i = 0; i<num_threads; i++){
		sops[0].sem_num = i;
		sops[0].sem_op = -1;

main_semread_wait:
		errno = 0;
		if(semop(sem_read, sops, 1) == -1){
			if(errno == EINTR) goto main_semread_wait;
			else{
				printf("Main thread couldn't get token from sem_read\n\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
			}
read_input:
		errno = 0;
		if(fgets(buf[i], LINE_SIZE, stdin) == 0){
			if(errno == EINTR) goto read_input;
			else{
				printf("Couldn't read input\n\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
			}

		sops[0].sem_op = 1;
main_semwritten_signal:
		errno = 0;
		if(semop(sem_written, sops, 1) == -1){
			if(errno == EINTR) goto main_semwritten_signal;
			else{
				printf("Couldn't put toke into sem_read\n\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
			}
		}
	}
}
