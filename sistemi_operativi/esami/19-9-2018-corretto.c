#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>


typedef struct thread_param{
	int i;
	int file_descriptor;}param;

int sem_written, sem_read;
int *fd;
char **buf;
int n;
char **filenames;

void handler(int sig){
	printf("Received SIGINT - I'm printing files\n\n");
	char output[6];
	output[5] = '\0';
	int rd;
	int file_d[n];
	for(int i = 0; i<n; i++){
		file_d[i] = open(filenames[i], O_RDONLY, 0666);
		if(file_d[i] == -1){
			printf("Signal open error\n\n");
			fflush(stdout);
			exit(EXIT_FAILURE);}
	}

	while(1){
	
		for(int i = 0; i<n; i++){
			rd = read(file_d[i], output, 5);
			if(rd == 0) break;
			printf("%s\n", output);}
		
		if(rd == 0) break;}
	
	for(int i = 0; i<n; i++){
		close(file_d[i]);}
}


void *writer(void *arg){
	param *p = (param *)arg;
	int me = p->i;
	int channel = p->file_descriptor;
	
	struct sembuf sem[1];
	while(1){
	int bytes_written = 0, remaining_bytes = 5;
	sem[0].sem_num = me;
	sem[0].sem_op = -1;
	sem[0].sem_flg = 0;
sem_wr2:
	errno = 0;
	if(semop(sem_written, sem, 1) == -1){
		if(errno == EINTR) goto sem_wr2;
		else{
			printf("Thread %d couldn't get token from sem_written\n\n", me);
			printf("%d\n\n", errno);
			exit(EXIT_FAILURE);}
	}
write:
	errno = 0;
	bytes_written = write(channel, &(buf[me][bytes_written]), remaining_bytes);
	if(sem_written == -1 && errno == EINTR) goto write;
	else if(sem_written == -1){
		printf("Couldn't write on file %d\n\n", me);
		exit(EXIT_FAILURE);}
	else if(sem_written < 5){
		remaining_bytes = remaining_bytes - bytes_written;
		goto write;}
	printf("Thread %d wrote on file\n\n", me);
	fflush(stdout);

	sem[0].sem_num = me;
	sem[0].sem_op = 1;
	sem[0].sem_flg = 0;
sem_rd2:
	errno = 0;
	if(semop(sem_read, sem, 1) == -1){
		if(errno == EINTR) goto sem_rd2;
		else{
			printf("Couldn't put token into sem_read %d\n\n", me);
			fflush(stdout);
			printf("%d\n", errno);
			fflush(stdout);
			exit(EXIT_FAILURE);}
		}
	}
}

int main(int argc, char **argv){
	if(argc < 2){
		printf("Usage: prog <filename1> <filename2> ...\n\n");
		exit(EXIT_FAILURE);}
	
	n = argc-1;
	pthread_t tid;

	
	buf = (char **)malloc(sizeof(char *) * n);
	if (buf == NULL){
		printf("Buf malloc error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	for(int i = 0; i<n; i++){
		buf[i] = (char *)malloc(sizeof(char *));
		if(buf[i] == NULL){
			printf("Couldn't allocate buf[%d]\n\n", i);
			fflush(stdout);
			exit(EXIT_FAILURE);}
	}

	sem_read = semget(IPC_PRIVATE, n, IPC_CREAT | 0666);
	for(int i=0; i<n; i++){
		if(semctl(sem_read, i, SETVAL, 1) == -1){
			printf("Semaphore error\n\n");
			fflush(stdout);
			exit(EXIT_FAILURE);}
	}

	sem_written = semget(IPC_PRIVATE, n, IPC_CREAT | 0666);
	for(int i = 0; i<n; i++){
		if(semctl(sem_written, i, SETVAL, 0) == -1){
			printf("Semaphore written error\n\n");
			fflush(stdout);
			exit(EXIT_FAILURE);}
	}

	if(signal(SIGINT, handler) == SIG_ERR){
		printf("Signal error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	fd = malloc(sizeof(int *) * n);
	if(fd == NULL){
		printf("Malloc error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	filenames = malloc(sizeof(char *) * n);
	if(filenames == NULL){
		printf("Filename malloc error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	for(int i = 0; i < n; i++){
		filenames[i] = argv[i+1];
		fd[i] = open(argv[i+1], O_CREAT | O_RDWR | O_TRUNC, 0666);
		if(fd[i] == -1){
			printf("Couldn't open file %d\n\n", i);
			fflush(stdout);
			exit(EXIT_FAILURE);}
		}

	for(int j = 0; j<n; j++){
		param *arg = (param *)malloc(sizeof(param));
		if(arg == NULL){
			printf("Malloc-thread error\n\n");
			fflush(stdout);
			exit(EXIT_FAILURE);}
		arg->i=j;
		arg->file_descriptor=fd[j];
		if(pthread_create(&tid, NULL, writer, (void *)arg) == -1){
			printf("Couldn't create thread %d\n\n", j);
			fflush(stdout);
			exit(EXIT_FAILURE);}
	}
	while(1){
		for(int i = 0; i<n; i++){
			int bytes_read = 0, remaining_bytes = 5;
			struct sembuf sops[1];
			sops[0].sem_num = i;
			sops[0].sem_op = -1;
			sops[0].sem_flg = 0;
sem_rd:
			errno = 0;
			if(semop(sem_read, sops, 1) == -1){
				if(errno = EINTR) goto sem_rd;
				else{
					printf("Couldn't take token from sem_read\n\n");
					fflush(stdout);
					exit(EXIT_FAILURE);}
			}
read_5_bytes:
			errno = 0;
			bytes_read = read(0,&(buf[i][bytes_read]), remaining_bytes);
			if(bytes_read == -1 && errno == EINTR) goto read_5_bytes;
			else if(bytes_read == -1){
				printf("Couldn't read input\n\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
			else if(remaining_bytes > 0 && remaining_bytes <= 5){
				remaining_bytes = remaining_bytes - bytes_read;
				goto read_5_bytes;}
			printf("Thread %d received message %s\n", i, buf[i]);
			fflush(stdout);

			sops[0].sem_op = 1;
sem_wr:
			errno = 0;
			if(semop(sem_written, sops, 1) == -1){
				if(errno == EINTR) goto sem_wr;
				else{
					printf("Couldn't put token in sem_written\n\n");
					fflush(stdout);
					exit(EXIT_FAILURE);}
			}
		}
	}	
}
