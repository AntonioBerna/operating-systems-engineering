/******************************************************************
Welcome to the Operating System examination

You are editing the '/home/esame/prog.c' file. You cannot remove 
this file, just edit it so as to produce your own program according to
the specification listed below.

In the '/home/esame/'directory you can find a Makefile that you can 
use to compile this prpogram to generate an executable named 'prog' 
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
Implementare un programma in linguaggio C che riceva in input, tramite argv[], il nome
di un file F. Il programa dovra' creare il file F e popolare il file
con lo stream proveniente da standard-input. Il programma dovra' generare
anche un ulteriore processo il quale dovra' riversare il contenuto che
viene inserito in F su un altro file denominato shadow_F, tale inserimento
dovra' essere realizzato in modo concorrente rispetto all'inserimento dei dati su F.

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando un qualsiasi processo (parent o child) venga colpito si
dovra' immediatamente emettere su standard-output il contenuto del file 
che il processo child sta popolando.

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

sem_t *s1, *s2;

const char *filename;
char *shadow_filename;

pid_t pid;

int interrupt_flag = 0;

void signal_handler(int sig) {
    if (sig == SIGINT && pid == 0) {
        int shadow_fd = open(shadow_filename, O_RDONLY);
        if (shadow_fd == -1) {
            perror("shadow_fd");
            exit(EXIT_FAILURE);
        }

        char buffer[BUFSIZ];
        ssize_t bytes_read, bytes_written;
        while ((bytes_read = read(shadow_fd, buffer, sizeof(buffer))) > 0) {
            bytes_written = write(STDOUT_FILENO, buffer, bytes_read);
            if (bytes_written == -1) {
                perror("bytes_written");
                exit(EXIT_FAILURE);
            }
        }
        close(shadow_fd);
    }
}

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

void child(void) {
    while (!interrupt_flag) {
        sem_wait(s1);
        copy_file(shadow_filename, filename);
        // printf("pong\n");
        sem_post(s2);
    }
}

void parent(void) {
    while (!interrupt_flag) {
        sem_wait(s2);
        char buffer[BUFSIZ];
        ssize_t bytes_read, bytes_written;
        bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
        if (bytes_read > 0) {
            int fd = open(filename, O_WRONLY | O_APPEND, 0644);
            if (fd == -1) {
                perror("fd");
                exit(EXIT_FAILURE);
            }
            bytes_written = write(fd, buffer, bytes_read);
            if (bytes_written == -1) {
                perror("bytes_written");
                exit(EXIT_FAILURE);
            }
            close(fd);
            // printf("ping\n");
            sem_post(s1);
        }
    }
}

int main(int argc, const char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", *argv);
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    s1 = mmap(NULL, sizeof(*s1), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    s2 = mmap(NULL, sizeof(*s2), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    sem_init(s1, 1, 0);
    sem_init(s2, 1, 1);

    filename = *(argv + 1);
    size_t shadow_filename_length = strlen(SHADOW_FILENAME_PREFIX) + strlen(filename) + 1;
    shadow_filename = malloc(shadow_filename_length);
    snprintf(shadow_filename, shadow_filename_length, "%s%s", SHADOW_FILENAME_PREFIX, filename);

    int fd = open(filename, O_TRUNC, 0644);
    if (fd == -1) {
        perror("fd");
        exit(EXIT_FAILURE);
    }
    close(fd);

    pid = fork();
    if (pid == -1) {
        perror("fork");

        sem_destroy(s1);
        sem_destroy(s2);

        munmap(s1, sizeof(*s1));
        munmap(s2, sizeof(*s2));

        free(shadow_filename);

        exit(EXIT_FAILURE);
    } else if (pid == 0) { // Child process
        child();
    } else { // Parent process
        parent();
    }

    sem_destroy(s1);
    sem_destroy(s2);

    munmap(s1, sizeof(*s1));
    munmap(s2, sizeof(*s2));

    free(shadow_filename);

    return 0;
}
