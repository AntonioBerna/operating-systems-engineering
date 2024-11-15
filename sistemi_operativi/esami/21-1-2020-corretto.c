#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/sem.h>
#include <errno.h>
#include <sys/ipc.h>
#include <string.h>
#include <fcntl.h>
#include <sys/syscall.h>


char filename1[100];
char filename2[100];
int fd[2];
int sem_written;
int sem_read;
char buff[6] = {0};


void *direct(void *p){
	struct sembuf sops[0];
	sops[0].sem_num = 0;
	sops[0].sem_flg = 0;
	while(1){
		sops[0].sem_op = -1;
direct_sem_written_wait:
		errno = 0;
		if(semop(sem_written, sops, 1) == -1){
			if(errno == EINTR) goto direct_sem_written_wait;
			else{
				printf("Direct thread couldn't get token form sem_written\n\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
			}
		printf("Direct thread received %s\n", buff);

		int prev_bytes_written = 0, bytes_written = 0, remaining_bytes = 5;
direct_file_writing:
		errno = 0;
		bytes_written = write(fd[0], &(buff[prev_bytes_written]), remaining_bytes);
		if(bytes_written == -1 && errno == EINTR) goto direct_file_writing;
		else if(bytes_written != remaining_bytes){
			prev_bytes_written = prev_bytes_written + bytes_written;
			remaining_bytes = remaining_bytes - bytes_written;
			goto direct_file_writing;}
		else if(bytes_written == -1){
			printf("Direct thread couldn't write on direct file\n\n");
			fflush(stdout);
			exit(EXIT_FAILURE);}

		sops[0].sem_op = 1;
direct_sem_read_signal:
		errno = 0;
		if(semop(sem_read, sops, 1) == -1){
			if(errno == EINTR) goto direct_sem_read_signal;
			else{
				printf("Direct thread couldn't put token into sem_read\n\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
		}
	}
}


void *reverse(void *p){
	struct sembuf sops[1];
	sops[0].sem_num = 0;	
	sops[0].sem_flg = 0;
	char temp1[6] = {0};
	char temp2[6] = {0};
	char *rdwr[2];
	rdwr[0] = temp1;
	rdwr[1] = temp2;
	int i = 0;	//points to the buffer to be written on file

	while(1){
		sops[0].sem_op = -1;
reverse_sem_written_wait:
		errno = 0;
		if(semop(sem_written, sops, 1) == -1){
			if(errno == EINTR) goto reverse_sem_written_wait;
			else{
				printf("Reverse thread couldn't take token from sem_written\n\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
		}
	
		memcpy(rdwr[i], buff, 5);
		printf("Reverse thread received %s\n", rdwr[i]);
		fflush(stdout);
		sops[0].sem_op = 1;
reverse_sem_read_signal:
		errno = 0;
		if(semop(sem_read, sops, 1) == -1){
			if(errno == EINTR) goto reverse_sem_read_signal;
			else{
				printf("Reverse thread couldn't put token into sem_read\n\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
		}
		lseek(fd[1], 0, SEEK_SET);
		int prev_bytes_read, bytes_read, prev_bytes_written, bytes_written, remaining_bytes;
		do{
			prev_bytes_read = 0; 
			bytes_read = 0; 
			remaining_bytes = 5;
	reverse_read:
			errno = 0;
			bytes_read = read(fd[1], &(rdwr[(i+1)%2][prev_bytes_read]), remaining_bytes);
			if(bytes_read == -1 && errno == EINTR) goto reverse_read;
			else if(bytes_read == -1){
				printf("Reverse thread couldn't read form reverse file\n\n");
				printf("%d\n", errno);
				fflush(stdout);
				exit(EXIT_FAILURE);}
			else if(bytes_read != remaining_bytes && bytes_read != 0){
				prev_bytes_read += bytes_read;
				remaining_bytes -= bytes_read;
				goto reverse_read;}
			if(remaining_bytes == 0 || bytes_read == 5){
				lseek(fd[1], -5, SEEK_CUR);}

			prev_bytes_written = 0;
			bytes_written = 0;
			remaining_bytes = 5;
	reverse_write:
			errno = 0;
			bytes_written = write(fd[1], &(rdwr[i][prev_bytes_written]), remaining_bytes);
			if(bytes_written == -1 && errno == EINTR) goto reverse_write;
			else if(bytes_written != remaining_bytes){
				prev_bytes_written += bytes_written;
				remaining_bytes -= bytes_written;
				goto reverse_write;}
			else if(bytes_written == -1){
				printf("Revesre couldn't write on file\n\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}

			i = (i+1) % 2;
		}while(bytes_read != 0);
	}
}



void *counter(void *p){
	sigset_t set;
	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, NULL);
	char buff_d[1];
	char buff_r[1];
	int count = 0;
	int file_des[2];

	file_des[0] = open(filename1,O_RDONLY, 0666);
	file_des[1] = open(filename2, O_RDONLY, 0666);
	if(file_des[0] == -1 || file_des[1] == -1){
		printf("Counter thread couldn't open file\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	
	while(read(file_des[0], buff_d, 1) != 0){
		if(read(file_des[1], buff_r, 1) == 0) break;
		if(buff_d[0] != buff_r[0]) count++;}
	printf("There are %d different characters\n", count);
	fflush(stdout);
	close(file_des[0]);
	close(file_des[1]);
	sigprocmask(SIG_UNBLOCK, &set, NULL);
}


void handler(int sign){
	pthread_t tid;
	if(pthread_create(&tid, NULL, counter, NULL) == -1){
		printf("Handler couldn't create counter thread\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	}

int main(int argc, char **argv){

	if(argc != 2){
		printf("Usage: prog <string S>\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	
	pthread_t tid;
	struct sigaction sa;
	sigset_t set;

	sprintf(filename1, "%s_diretto.txt", argv[1]);
	sprintf(filename2, "%s_inverso.txt", argv[1]);

	fd[0] = open(filename1, O_CREAT | O_TRUNC | O_RDWR, 0666);
	fd[1] = open(filename2, O_CREAT | O_TRUNC | O_RDWR, 0666);
	if(fd[0] == -1 || fd[1] == -1){
		printf("Couldn't open file\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	sem_written = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
	if(sem_written == -1){
		printf("Sem_written creation error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	if(semctl(sem_written, 0, SETVAL, 0) == -1){
		printf("Sem_written initialization error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	sem_read = semget(IPC_PRIVATE, 1, IPC_CREAT | 0666);
	if(sem_read == -1){
		printf("Sem_read creation error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	if(semctl(sem_read, 0, SETVAL, 2) == -1){
		printf("Sem_read initialization error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	if(pthread_create(&tid, NULL, direct, NULL) == -1){
		printf("Thread direct creation error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}
	if(pthread_create(&tid, NULL, reverse, NULL) == -1){
		printf("Thread reverse creation error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	memset(&sa, 0, sizeof(sa));
	sigfillset(&set);
	sa.sa_handler = handler;
	sa.sa_mask = set;
	if(sigaction(SIGINT, &sa, NULL) == -1){
		printf("Sigaction error\n\n");
		fflush(stdout);
		exit(EXIT_FAILURE);}

	struct sembuf sops[1];
	sops[0].sem_num = 0;
	sops[0].sem_flg = 0;

	while(1){
		sops[0].sem_op = -2;
main_sem_read_wait:
		errno = 0;
		if(semop(sem_read, sops, 1) == -1){
			if(errno == EINTR) goto main_sem_read_wait;
			else{
				printf("Main thread couldn't get tokens from sem_read\n\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
		}

		int prev_bytes_read = 0, bytes_read = 0, remaining_bytes = 5;
read_from_stdin:
		errno = 0;
		bytes_read = read(0, &(buff[prev_bytes_read]), remaining_bytes);
		if(bytes_read == -1 && errno == EINTR) goto read_from_stdin;
		else if(bytes_read == -1){
			printf("Couldn't read from stdin\n\n");
			fflush(stdout);
			exit(EXIT_FAILURE);}
		else if(bytes_read != remaining_bytes){
			prev_bytes_read = prev_bytes_read + bytes_read;
			remaining_bytes = remaining_bytes - bytes_read;
			goto read_from_stdin;}
		printf("Main thread read %s\n", buff);

		sops[0].sem_op = 2;
main_sem_written_signal:
		errno = 0;
		if(semop(sem_written, sops, 1) == -1){
			if(errno == EINTR) goto main_sem_written_signal;
			else{
				printf("Couldn't put tokens into sem_written\n\n");
				fflush(stdout);
				exit(EXIT_FAILURE);}
			}
		}
}
