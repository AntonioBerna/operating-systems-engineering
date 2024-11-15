#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>


#define LINE_SIZE 10

char **filenames;
char **strings;
int *fd;
int num_proc;
char *delim = ",";
pid_t pid;
int num_file;
char command[100];

void int_parent_handler(int sign){
	printf("Parent proc received SIGINT\n");
	fflush(stdout);
	num_file = (num_file+1) % num_proc;
	printf("Now I'm writing on %d file\n", num_file);
	fflush(stdout);}



void int_child_handler(int sign){
	printf("Proc child %d received SIGINT\n", getpid());
	fflush(stdout);
	system(command);}


int main(int argc, char **argv){
	
	if(argc < 2){
		printf("Usage: prog <couple1> <couple2> ...\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	num_proc = argc - 1;
	int i;
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sigset_t set;

	filenames = (char **)malloc(sizeof(char *) * num_proc);
	if(filenames == NULL){
		printf("Filenames malloc error\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	strings = (char **)malloc(sizeof(char *) * num_proc);
	if(strings == NULL){
		printf("Strings malloc error\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	for(i = 0; i<num_proc; i++){
		filenames[i] = strtok(argv[i+1], delim);
		strings[i] = strtok(NULL, delim);
		if(strings[i] == NULL || filenames[i] == NULL){
			printf("Couple syntax is: <filename,string>\n");
			fflush(stdout);
			exit(EXIT_FAILURE);}
	}


	fd = (int *)malloc(sizeof(int ) * num_proc);
	if(fd == NULL){
		printf("Fd malloc error\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}


	for(i = 0; i<num_proc; i++){	
		fd[i] = open(filenames[i], O_CREAT | O_TRUNC | O_WRONLY, 0666);
		if(fd[i] == -1){
			printf("fd[%d] open error\n", i);
			fflush(stdout);
			exit(EXIT_FAILURE);}
	}


	for(i=0; i<num_proc; i++){
		pid = fork();
		if(pid == -1){
			printf("Fork error\n");
			fflush(stdout);
			exit(EXIT_FAILURE);}
		else if(pid == 0) break;
	}


	if(pid == 0){
		setpgid(0, 0);
		sprintf(command, "cat %s\n", filenames[i]);

		char buff;
		int count;
		int bytes_read;
			
			
		sigfillset(&set);
		sa.sa_handler = int_child_handler;
		sa.sa_mask = set;
		if(sigaction(SIGINT, &sa, NULL) == -1){	
			printf("Child proc %d sigaction error\n", getpid());
			fflush(stdout);
			exit(EXIT_FAILURE);}
			
		while(1){
			count = 0;
			sleep(5);
			int file_desc = open(filenames[i], O_RDONLY, 0666);
			if(file_desc == -1){
				printf("Proc child %d couldn't open file\n", getpid());
				fflush(stdout);
				exit(EXIT_FAILURE);}
			
			int j = 0;
read_from_file:

			errno = 0;
			bytes_read = read(file_desc, &buff, 1);
			if(bytes_read == -1 && errno == EINTR) goto read_from_file;
			else if(bytes_read == -1){
				printf("Proc child %d couldn't read from file\n", getpid());
				fflush(stdout);
				exit(EXIT_FAILURE);}

			if(bytes_read != 0){
				if(buff == strings[i][j]){
					j++;
					if(strings[i][j] == '\0'){
						count++;
						j = 0;}
				}
				else {
					
					if(j == 1) lseek(file_desc, -1, SEEK_CUR);
					j = 0;}
				
				goto read_from_file;}


			printf("Process %d found %d occurencies of %s\n", getpid(), count, strings[i]);
			fflush(stdout);
			close(file_desc);
		
	}
}
	else if(pid != 0){
		sigfillset(&set);
		sa.sa_handler = int_parent_handler;
		sa.sa_mask = set;
		if(sigaction(SIGINT, &sa, NULL) == -1){
			printf("Parent proc sigaction error\n");
			fflush(stdout);
			exit(EXIT_FAILURE);}
		
		char buff[LINE_SIZE] = {0};
		int prev_bytes_written, prev_bytes_read, bytes_written, bytes_read, remaining_bytes;		
	while(1){
		remaining_bytes = LINE_SIZE-1;
		prev_bytes_read = bytes_read = 0;
read_from_stdin:
		errno = 0;
		bytes_read = read(0, &buff[prev_bytes_read], remaining_bytes);
		if(bytes_read == -1 && errno == EINTR) goto read_from_stdin;
		else if(bytes_read == -1){
			printf("Proc parent couldn't read from stdin\n");
			fflush(stdout);
			exit(EXIT_FAILURE);}
		else if(bytes_read < remaining_bytes){
			prev_bytes_read += bytes_read;
			remaining_bytes -= bytes_read;
			goto read_from_stdin;}
		printf("Parent proc read %s - writing on file %d\n", buff, num_file);
		fflush(stdout);

		remaining_bytes = LINE_SIZE-1;
		prev_bytes_written = bytes_written = 0;
write_on_file:
		errno = 0;
		bytes_written = write(fd[num_file], &buff[prev_bytes_written], remaining_bytes);
	        if(bytes_written == -1 && errno == EINTR) goto write_on_file;
		else if(bytes_written == -1){
			printf("Proc parent couldn't read on file %d\n", num_file);
			fflush(stdout);
			exit(EXIT_FAILURE);}
		else if(bytes_written < remaining_bytes){
			prev_bytes_written += bytes_written;
			remaining_bytes -= bytes_written;
			goto write_on_file;}	
		}
	}

}
