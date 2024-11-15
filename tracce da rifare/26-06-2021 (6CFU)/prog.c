/******************************************************************
Welcome to the Operating System examination

You are editing the '/home/esame/prog.c' file. You cannot remove 
this file, just edit it so as to produce your own program according to
the specification listed below.

In the '/home/esame/'directory you can find a Makefile that you can 
use to compile this program to generate an executable named 'prog' 
in the same directory. Typing 'make posix' you will compile for 
Posix, while typing 'make winapi' you will compile for WinAPI just
depending on the specific technology you selected to implement the
given specification. Most of the required header files (for either 
Posix or WinAPI compilation) are already included in the head of the
prog.c file you are editing. 

At the end of the examination, the last saved snapshot of this file
will be automatically stored by the system and will be then considered
for the evaluation of your exam. Moifications made to prog.c which are
not saved by you via the editor will not appear in the stored version
of the prog.c file. 
In other words, unsaved changes will not be tracked, so please save 
this file when you think you have finished software development.
You can also modify the Makefile if requesed, since this file will also
be automatically stored together with your program and will be part
of the final data to be evaluated for your exam.

PLEASE BE CAREFUL THAT THE LAST SAVED VERSION OF THE prog.c FILE (and of
the Makfile) WILL BE AUTOMATICALLY STORED WHEN YOU CLOSE YOUR EXAMINATION 
VIA THE CLOSURE CODE YOU RECEIVED, OR WHEN THE TIME YOU HAVE BEEN GRANTED
TO DEVELOP YOUR PROGRAM EXPIRES. 


SPECIFICATION TO BE IMPLEMENTED:
Implementare una programma che riceva in input, tramite argv[], il nome
di un file F. Il programa dovra' creare il file F e popolare il file
con lo stream proveniente da standard-input. Il programma dovra' generare
anche un ulteriore processo il quale dovra' riversare il contenuto del file F
su un altro file denominato shadow_F, inserendo mano a mano in shadow_F soltanto 
i byte che non erano ancora stati prelevati in precedenza da F. L'operazione 
di riversare i byte di F su shadow_F dovra' avvenire con una periodicita' di 10
secondi.  

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando un qualsiasi processo (parent o child) venga colpito si
dovra' immediatamente riallineare il contenuto del file shadow_F al contenuto
del file F sempre tramite operazioni che dovra' eseguire il processo child.

Qualora non vi sia immissione di input, l'applicazione dovra' utilizzare 
non piu' del 5% della capacita' di lavoro della CPU.

*****************************************************************/

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

#define SHADOW_FILENAME_PREFIX "shadow_"

const char *filename;
char *shadow_filename;

int interrupt_flag = 0;

void copy_file(const char *dst_filename, const char *src_filename) {
    int src_fd = open(src_filename, O_RDONLY);
    if (src_fd == -1) {
        perror("src_fd");
        exit(EXIT_FAILURE);
    }

    int dst_fd = open(dst_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd == -1) {
        perror("dst_fd");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFSIZ];
    ssize_t bytes_read, bytes_written;
    while ((bytes_read = read(src_fd, buffer, BUFSIZ)) > 0) {
        bytes_written = write(dst_fd, buffer, bytes_read);
        if (bytes_written == -1) {
            perror("bytes_written");
            exit(EXIT_FAILURE);
        }
    }

    if (bytes_read == -1) {
        perror("bytes_read");
        exit(EXIT_FAILURE);
    }

    close(src_fd);
    close(dst_fd);
}

void handle_signal(int sig) {
    if (sig == SIGINT) {
        copy_file(shadow_filename, filename);
    }
}

int main(int argc, const char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", *argv);
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, handle_signal) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    filename = *(argv + 1);
    size_t shadow_filename_length = strlen(SHADOW_FILENAME_PREFIX) + strlen(filename) + 1;
    shadow_filename = malloc(shadow_filename_length);
    snprintf(shadow_filename, shadow_filename_length, "%s%s", SHADOW_FILENAME_PREFIX, filename);

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        perror("fd");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        free(shadow_filename);
        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Child process
        while (!interrupt_flag) {
            copy_file(shadow_filename, filename);
            sleep(10);
        }
    } else { // Parent process
        char buffer[BUFSIZ];
        ssize_t bytes_read, bytes_written;
        while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
            bytes_written = write(fd, buffer, bytes_read);
            if (bytes_written == -1) {
                perror("bytes_written");
                exit(EXIT_FAILURE);
            }
        }
    }

    free(shadow_filename);
    return 0;
}
