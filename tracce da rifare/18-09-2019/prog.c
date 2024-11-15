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
for the evaluation of your exam. Modifications made to prog.c which are
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
Implementare una programma in linguaggo C che riceva in input, tramite argv[], un insieme di
stringhe S_1 ..... S_n con n maggiore o uguale ad 1. 
Per ogni stringa S_i dovra' essere attivato un thread T_i.
Il main thread dovra' leggere indefinitamente stringhe dallo standard-input.
Ogni stringa letta dovra' essere resa disponibile al thread T_1 che dovra' 
eliminare dalla stringa ogni carattere presente in S_1, sostituendolo con il 
carattere 'spazio'.
Successivamente T_1 rendera' la stringa modificata disponibile a T_2 che dovra' 
eseguire la stessa operazione considerando i caratteri in S_2, e poi la passera' 
a T_3 (che fara' la stessa operazione considerando i caratteri in S_3) e cosi' 
via fino a T_n. 
T_n, una volta completata la sua operazione sulla stringa ricevuta da T_n-1, dovra'
passare la stringa ad un ulteriore thread che chiameremo OUTPUT il quale dovra' 
stampare la stringa ricevuta su un file di output dal nome output.txt.
Si noti che i thread lavorano secondo uno schema pipeline, sono ammesse quindi 
operazioni concorrenti su differenti stringhe lette dal main thread dallo 
standard-input.

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra' 
stampare il contenuto corrente del file output.txt su standard-output.

In caso non vi sia immissione di dati sullo standard-input, l'applicazione 
dovra' utilizzare non piu' del 5% della capacita' di lavoro della CPU.

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

char **buffers;
int num_threads;
char **strings;
FILE *file;

pthread_mutex_t *start;
pthread_mutex_t *end;

void *routine(void *arg) {
    long int index = (long int)arg;

    while (1) {
        if (pthread_mutex_lock(start + index)) {
            perror("pthread_mutex_lock");
            exit(EXIT_FAILURE);
        }

        printf("thread: %ld - string: %s\n", index, buffers[index]);
        if (index == num_threads - 1) {
            fprintf(file, "%s\n", buffers[index]);
            fflush(file);
        } else {
            for (int i = 0; i < strlen(strings[index]); i++) {
                for (int j = 0; j < strlen(buffers[index]); j++) {
                    if (*(buffers[index] + j) == *(strings[index] + i)) {
                        *(buffers[index] + j) = ' ';
                    }
                }
            }

            if (pthread_mutex_lock(end + index + 1)) {
                perror("pthread_mutex_lock");
                exit(EXIT_FAILURE);
            }

            buffers[index + 1] = buffers[index];

            if (pthread_mutex_unlock(start + index + 1)) {
                perror("pthread_mutex_unlock");
                exit(EXIT_FAILURE);
            }
        }

        if (pthread_mutex_unlock(end + index)) {
            perror("pthread_mutex_unlock");
            exit(EXIT_FAILURE);
        }
    }
}

void signal_handler(int arg) {
    (void)arg;
    puts("");
    system("cat output.txt");
}

int main(int argc, char **argv) {
    int ret;
    char *p;
    long int i;
    pthread_t tid;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <S_1> ... <S_n>\n", *argv);
        exit(EXIT_FAILURE);
    }

    file = fopen("output.txt", "w+");
    if (file == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    strings = argv + 1;
    num_threads = argc;
    buffers = (char **)malloc(num_threads * sizeof(char *));
    if (buffers == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    start = malloc(num_threads * sizeof(pthread_mutex_t));
    end = malloc(num_threads * sizeof(pthread_mutex_t));
    if (start == NULL || end == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    for (i = 0; i < num_threads; i++) {
        if (pthread_mutex_init(start + i, NULL) || pthread_mutex_init(end + i, NULL) || pthread_mutex_lock(start + i)) {
            perror("pthread_mutex_init");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < num_threads; i++) {
        if (pthread_create(&tid, NULL, &routine, (void *)i) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("signal");
        exit(EXIT_FAILURE);
    }

    while (1) {
        redo_again:
        ret = scanf("%ms", &p);
        if (ret == EOF || errno == EINTR) {
            perror("scanf");
            goto redo_again;
        }

        redo_end:
        if (pthread_mutex_lock(end)) {
            if (errno == EINTR) {
                goto redo_end;
            }
            exit(EXIT_FAILURE);
        }

        buffers[0] = p;

        redo_start:
        if (pthread_mutex_unlock(start)) {
            if (errno == EINTR) {
                goto redo_start;
            }
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}