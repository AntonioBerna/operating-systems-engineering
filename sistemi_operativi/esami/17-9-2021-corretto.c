#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

#define LINE_SIZE 1024

char filename[100];
char file_shadow[100];
int bytes_read = 0;
int global_count = 0;
char string[100];

void *handler_thread(void *p){
	
	printf("Thread created\n");
	fflush(stdout);
	sigset_t set;
	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, NULL);

	FILE *fs = fopen(file_shadow, "a+");
	if(fs == NULL){
		printf("Couldn't open file shadow\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	FILE *ft = fopen(filename, "r");
	if(ft == NULL){
		printf("Couldn't open file\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	fseek(ft, bytes_read, SEEK_SET);


	char buf[LINE_SIZE] = {0};
	int len, local_count = 0;

	while(fgets(buf, LINE_SIZE - 1, ft) != 0){
		len = strlen(buf);
		buf[len - 1] = '\0';
		if(strcmp(buf, string) == 0){
			local_count++;
			global_count++;
			for(int i = 0; i<strlen(string); i++){
				buf[i] = '*'; }
		}
		buf[len-1] = '\n';
		fprintf(fs, "%s", buf);
		fflush(fs);
		bytes_read += len;
	}
	fclose(ft);
	fclose(fs);
	printf("Strings found in the whole file: %d\n", global_count);
	fflush(stdout);
	printf("Strings find in the last transfer: %d\n", local_count);
	fflush(stdout);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
}


void int_handler(int sign){
	printf("Received SIGINT - creating thread\n");
	fflush(stdout);

	pthread_t tid;
	if(pthread_create(&tid, NULL, handler_thread, NULL) != 0){
		printf("Handler couldn't create thread\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
}



int main(int argc, char **argv){
	
	if(argc != 3){
		printf("Usage: prog <file> <string>\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}	
	

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sigset_t set;
	sigfillset(&set);

	memcpy(filename, argv[1], strlen(argv[1]) + 1);
	memcpy(string, argv[2], strlen(argv[2]) + 1);

	sprintf(file_shadow, "%s_shadow", argv[1]);
	FILE *fd = fopen(filename, "w");
	if(fd == NULL){
		printf("File opening error\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	
	sa.sa_handler = int_handler;
	sa.sa_mask = set;
	if(sigaction(SIGINT, &sa, NULL) == -1){
		printf("Sigaction error\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	
	
	char buff[LINE_SIZE] = {0};
	while(1){
read_from_stdin:
		errno = 0;
		if(fgets(buff, LINE_SIZE-1, stdin) == 0){
			if(errno == EINTR) goto read_from_stdin;
			else{
				printf("Couldn't read from stdin\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
		}

		printf("Main thread read %s - writing on file\n", buff);
		fflush(stdout);

		fprintf(fd, "%s", buff);
		fflush(fd);
	}
}
