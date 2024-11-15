#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <signal.h>
#include <errno.h>
#include <string.h>


#define LINE_SIZE 10

int fd[2];
int file_desc;
pid_t pid;
int file, shadow;
char file_shadow[100];

void int_handler(int sign){
	char command[120] = {0};
	printf("%d process received SIGINT - printing file\n", getpid());
	sprintf(command, "cat %s\n", file_shadow);
	system(command);
	write(0, "\n", 1);}


int main(int argc, char **argv){
	
	if(argc != 2){
		printf("Usage: prog <filename>\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	
	sprintf(file_shadow, "shadow_%s", argv[1]);

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sigset_t set;
	sigfillset(&set);

	fd[0] = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY);
	fd[1] = open(file_shadow, O_CREAT | O_TRUNC | O_WRONLY);
	if(fd[0] == -1 || fd[1] == -1){
		printf("File creation error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	struct sembuf sops[1];
	file = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
	shadow = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
	if(file == -1 || shadow == -1){	
		printf("Couldn't create semaphores\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	if(semctl(file, 0, SETVAL, 0) == -1 || semctl(shadow, 0, SETVAL, 1) == -1){
		printf("Couldn't initialize semaphores\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	
	pid = fork();
	if(pid == -1){
		printf("Fork error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	else if(pid == 0){
	
		if(setpgid(0, 0) == -1){
			printf("Coudn't change group of child process - noth processes will recieve SIGINT at any time the signal it sent\n\n");
			fflush(stdout);}
	
		sa.sa_handler = int_handler;
		sa.sa_mask = set;
		if(sigaction(SIGINT, &sa, NULL) == -1){
			printf("Child process couldn't setup handler\n\n");
			fflush(stdout);
			kill(getppid(), SIGKILL);
			exit(EXIT_FAILURE);}
		
		file_desc = open(argv[1], O_RDONLY, 0666);
		if(file_desc == -1){
			printf("Child process couldn't open file\n\n");
			fflush(stdout);
			kill(getppid(), SIGKILL);
			exit(EXIT_FAILURE);}

		int prev_bytes_read, prev_bytes_written, bytes_read, bytes_written, remaining_bytes;
		char buff[LINE_SIZE+1] = {0};
		while(1){
			sops[0].sem_op = -1;
sem_file_wait:
			errno = 0;
			if(semop(file, sops, 1) == -1){
				if(errno == EINTR) goto sem_file_wait;
				else{	
					printf("Child process couldn't get token from sem file\n\n");
					fflush(stdout);
					kill(getppid(), SIGKILL);
					exit(EXIT_FAILURE);}
				}

			prev_bytes_read = bytes_read = 0;
			remaining_bytes = LINE_SIZE;
read_from_file:
			errno = 0;
			bytes_read = read(file_desc, &(buff[prev_bytes_read]), remaining_bytes);
			if(bytes_read == -1 && errno == EINTR) goto read_from_file;
			else if(bytes_read == -1){
				printf("Child process couldn't read from file\n\n");
				fflush(stdout);
				kill(getppid(), SIGKILL);
				exit(EXIT_FAILURE);}
				
			else if(bytes_read != remaining_bytes){
				prev_bytes_read += bytes_read;
				remaining_bytes -= bytes_read;
				goto read_from_file;}


			prev_bytes_written = bytes_written = 0;
			remaining_bytes = LINE_SIZE;
write_on_shadow:
			errno = 0;
			bytes_written = write(fd[1], &(buff[prev_bytes_written]), remaining_bytes);
			if(bytes_written == -1 && errno == EINTR) goto write_on_shadow;
			else if(bytes_written == -1){
				printf("Child process couldn't write on file shadow\n\n");
				fflush(stdout);
				kill(getppid(), SIGKILL);
				exit(EXIT_FAILURE);}
			else if(bytes_written != remaining_bytes){
				prev_bytes_written += bytes_written;
				remaining_bytes -= bytes_written;
				goto write_on_shadow;}

			sops[0].sem_op = 1;
sem_shadow_signal:
			errno = 0;
			if(semop(shadow, sops, 1) == -1){
				if(errno == EINTR) goto sem_shadow_signal;
				else{
					printf("Child process couldn't put token into sem shadow\n\n");
					fflush(stdout);
					kill(getppid(), SIGKILL);
					exit(EXIT_FAILURE);}
			}
		}
	}
	else{

		sa.sa_handler = int_handler;
		sa.sa_mask = set;
		if(sigaction(SIGINT, &sa, NULL) == -1){
			printf("Parent process couldn't setup handler\n\n");
			fflush(stdout);
			kill(pid, SIGKILL);
			exit(EXIT_FAILURE);}
		


		int prev_bytes_read, prev_bytes_written, bytes_read, bytes_written, remaining_bytes;
		char buff[LINE_SIZE + 1] = {0};
		sops[0].sem_num = 0;
		sops[0].sem_flg = 0;
		while(1){

		
		prev_bytes_read = bytes_read = 0;
		remaining_bytes = LINE_SIZE;
read_from_stdin:
		errno = 0;
		bytes_read = read(0, &(buff[prev_bytes_read]), remaining_bytes);
		if(bytes_read == -1 && errno == EINTR) goto read_from_stdin;
		else if(bytes_read == -1){
			printf("Parent process couldn't read from stdin\n\n");
			fflush(stdout);
			kill(pid, SIGKILL);
			exit(EXIT_FAILURE);}
		else if(bytes_read != remaining_bytes){
					prev_bytes_read += bytes_read;
					remaining_bytes -= bytes_read;
					goto read_from_stdin;}
		printf("Parent process read %s\n", buff);

		
		sops[0].sem_op = -1;
sem_shadow_wait:
		errno = 0;
		if(semop(shadow, sops, 1) == -1){
			if(errno == EINTR) goto sem_shadow_wait;
			else{
				printf("Parent process couldn't take token from shadow sem\n\n");
				fflush(stdout);
				kill(pid, SIGKILL);
				exit(EXIT_FAILURE);}
			}

		prev_bytes_written = bytes_written = 0;
		remaining_bytes = LINE_SIZE;
write_on_file:
		errno = 0;
		bytes_written = write(fd[0], &(buff[prev_bytes_written]), remaining_bytes);
		if(bytes_written == -1 && errno == EINTR) goto write_on_file;
		else if(bytes_written == -1){
			printf("Parent process couldn't write on file\n\n");
			fflush(stdout);
			kill(pid, SIGKILL);
			exit(EXIT_FAILURE);}
		else if(bytes_written != remaining_bytes){
			prev_bytes_written += bytes_written;
			remaining_bytes -= bytes_written;
			goto write_on_file;}

		sops[0].sem_op = 1;
sem_file_signal:
		errno = 0;
		if(semop(file, sops, 1) == -1){
			if(errno == EINTR) goto sem_file_signal;
			else{
				printf("Parent process couldn't put token into sem shadow\n\n");
				fflush(stdout);
				kill(pid, SIGKILL);
				exit(EXIT_FAILURE);}
			}


		}
	}
}
