#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#define LINE_SIZE 1024

int fd;
int num_proc;
int count;
int sem;
FILE *f, *fa;
char filename[100];
char **strings;

void int_handler(int sign){
	printf("Received SIGINT\n\n");
	fflush(stdout);
	
	sigset_t set;
	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, NULL);

	pid_t pid;
	int count = 0;
	int status;
	int i = 0;
	struct sembuf sops[1];
	sops[0].sem_flg = 0;

	for(i = 0; i<num_proc; i++){
		pid = fork();

		if(pid ==-1){
			printf("Fork error\n\n");
			fflush(stdout);
			exit(EXIT_FAILURE);
			}
		else if(pid == 0) break;
		}
	if(pid != 0){
	for(i = 0; i<num_proc; i++){
		 wait(&status);}
	}

	else if(pid == 0){

		char reverse[LINE_SIZE] = {0};
		char buff[LINE_SIZE] = {0};
		int len = strlen(strings[i]);
		
		for(int j = 0; j<len; j++){
			reverse[j] = strings[i][len - j - 1];}

		printf("Proc %d - reverse string is %s\n", i, reverse);
		fflush(stdout);

		sops[0].sem_num = 0;
		sops[0].sem_op = -1;
sem_wait:
		errno = 0;
		if(semop(sem, sops, 1) == -1){
			if(errno == EINTR) goto sem_wait;
			else{
				printf("Couldn't put token into sem\n\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
		}
		
		fseek(f, 0, SEEK_SET);
		while(fgets(buff, LINE_SIZE - 1, f) != 0){
			buff[strlen(buff) - 1] = '\0';
			if(strcmp(buff, reverse) == 0) count = count + 1;}
		printf("Process %d found %d reverse string of %s\n", i, count, strings[i]);
		fflush(stdout);

		sops[0].sem_op = 1;
sem_signal:
		errno = 0;
		if(semop(sem, sops, 1) == -1){
			if(errno == EINTR) goto sem_signal;
			else{
				printf("Couldn't put token into sem\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
		}
		exit(EXIT_SUCCESS);
	}
	fseek(f, 0, SEEK_SET);

	fa = fopen("backup.txt", "a+");
	if(fa == NULL){
		printf("Backup file error\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	
	printf("transferring data to backup file\n");
	fflush(stdout);	
	char temp[LINE_SIZE] = {0};
	while(fgets(temp, LINE_SIZE -1, f) != 0){
		fprintf(fa, "%s", temp);
		if(fflush(fa) == EOF){
			printf("Fflush error - %d\n", errno);
			fflush(stdout);
			exit(EXIT_FAILURE);}
	}

	fclose(fa);
	fclose(f);
	f=fopen(filename, "w+");
	if(f == NULL){
		printf("Handler couldn't open file\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	sigprocmask(SIG_UNBLOCK, &set, NULL);
		
}




int main(int argc, char **argv){
	
	if(argc < 3){	
		printf("Usage: prog <filename> <string1> <string2> ...\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sigset_t set;
	sigfillset(&set);

	num_proc = argc - 2;

	memcpy(filename, argv[1], strlen(argv[1]) + 1);
	strings = (char **)malloc(sizeof(char *) * num_proc);
	if(strings == NULL){
		printf("Strings malloc error\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	for(int i = 0; i<num_proc; i++){
		strings[i] = (char *)malloc(sizeof(char) * (strlen(argv[i+2] +1)));
		if(strings[i] == NULL){
			printf("Strings[%d] malloc error\n", i);
			fflush(stdout);
			exit(EXIT_FAILURE);}
		memcpy(strings[i], argv[i+2], strlen(argv[i+2]) + 1);
	}

	f = fopen(argv[1], "w+");
	if(f == NULL){
		printf("COuldn't open file\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	sem = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
	if(sem == -1){
		printf("Semget error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	if(semctl(sem, 0, SETVAL, 1) == -1){
		printf("Semctl error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	sa.sa_handler = int_handler;
	sa.sa_mask = set;
	if(sigaction(SIGINT, &sa, NULL) == -1){	
		printf("Sigaction error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	char buff[LINE_SIZE] = {0};
	while(1){
read_from_stdin:
		errno = 0;
		if(fgets(buff, LINE_SIZE - 1, stdin) == 0){
			if(errno == EINTR) goto read_from_stdin;
			else{
				printf("Fgets error\n\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
		}
		printf("Process read %s\n\n", buff);
		fflush(stdout);

		
write_on_file:
		errno = 0;
		int size = fprintf(f, "%s", buff);
		fflush(f);
		if(errno == EINTR && size == 0) goto write_on_file;
		else if(size == 0){	
				printf("Couldn't write on file\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
		
	}
}
