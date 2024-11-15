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
Implementare una programma che riceva in input, tramite argv[], il nome
di un file F e un insieme di N stringhe (con N almeno pari ad 1). Il programa dovra' creare 
il file F e popolare il file con le stringhe provenienti da standard-input.
Ogni stringa dovra' essere inserita su una differente linea del file.
L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito si
dovranno  generare N processi concorrenti ciascuno dei quali dovra' analizzare il contenuto
del file F e verificare, per una delle N stringhe di input, quante volte la inversa di tale stringa
sia presente nel file. Il risultato del controllo dovra' essere comunicato su standard
output tramite un messaggio. Quando tutti i processi avranno completato questo controllo, 
il contenuto del file F dovra' essere inserito in "append" in un file denominato "backup"
e poi il file F dova' essere troncato.

Qualora non vi sia immissione di input o di segnali, l'applicazione dovra' utilizzare 
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

#define BACKUP_FILENAME "backup"

const char *filename;
char *input_string;
sem_t *s1, *s2;

char *reverse(char *string) {
    size_t string_length = strlen(string);
    char *reversed_string = malloc((string_length + 1) * sizeof(*reversed_string));
    if (reversed_string == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < string_length; i++) {
        reversed_string[i] = string[string_length - i - 1];
    }
    reversed_string[string_length] = '\0';
    return reversed_string;
}

void signal_handler(int sig) {
    if (sig == SIGINT) {
        int fd = open(filename, O_RDONLY);
        if (fd == -1) {
            perror("fd");
            exit(EXIT_FAILURE);
        }

        char buffer[BUFSIZ];
        char *reversed_string = reverse(input_string);
        int *occurrences = mmap(NULL, sizeof(*occurrences), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
        *occurrences = 0;
        while (read(fd, buffer, sizeof(buffer)) > 0) {
            char *line = strtok(buffer, "\n");
            while (line != NULL) {
                pid_t pid = fork();
                if (pid == -1) {
                    sem_destroy(s1);
                    sem_destroy(s2);
                    munmap(s1, sizeof(*s1));
                    munmap(s2, sizeof(*s2));
                    munmap(occurrences, sizeof(*occurrences));
                    exit(EXIT_FAILURE);
                } else if (pid == 0) {
                    sem_wait(s2);
                    if (!strcmp(line, reversed_string)) {
                        (*occurrences)++;
                    }
                    sem_post(s1);
                    exit(EXIT_SUCCESS);
                } else {
                    sem_wait(s1);
                    line = strtok(NULL, "\n");
                    sem_post(s2);
                }

            }
        }
        printf("Occurrences of %s: %d\n", reversed_string, *occurrences);
        free(reversed_string);
        munmap(occurrences, sizeof(*occurrences));
        close(fd);

        fd = open(filename, O_RDONLY);
        if (fd == -1) {
            perror("fd");
            exit(EXIT_FAILURE);
        }

        int backup_fd = open(BACKUP_FILENAME, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (backup_fd == -1) {
            perror("backup_fd");
            exit(EXIT_FAILURE);
        }

        char backup_buffer[BUFSIZ];
        ssize_t backup_bytes_read, bytes_written;
        while ((backup_bytes_read = read(fd, backup_buffer, sizeof(backup_buffer))) > 0) {
            bytes_written = write(backup_fd, backup_buffer, backup_bytes_read);
            if (bytes_written == -1) {
                perror("bytes_written backup");
                exit(EXIT_FAILURE);
            }
        }
        close(backup_fd);
        close(fd);

        fd = open(filename, O_TRUNC, 0644);
        if (fd == -1) {
            perror("fd");
            exit(EXIT_FAILURE);
        }
        close(fd);
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <filename> <string-1> ... <string-n>\n", *argv);
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

    srand(time(NULL));
    filename = *(argv + 1);
    input_string = argv[rand() % (argc - 2) + 2];

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0644);
    if (fd == -1) {
        perror("fd");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFSIZ];
    ssize_t bytes_read, bytes_written;
    while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(fd, buffer, bytes_read);
        if (bytes_written == -1) {
            perror("bytes_written");
            exit(EXIT_FAILURE);
        }
    }
    close(fd);

    sem_destroy(s1);
    sem_destroy(s2);
    munmap(s1, sizeof(*s1));
    munmap(s2, sizeof(*s2));

    return 0;
}
