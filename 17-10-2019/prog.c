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
Implementare una programma che riceva in input, tramite argv[], il nomi
di N file (con N maggiore o uguale a 1).
Per ogni nome di file F_i ricevuto input dovra' essere attivato un nuovo thread T_i.
Il main thread dovra' leggere indefinitamente stringhe dallo standard-input 
e dovra' rendere ogni stringa letta disponibile ad uno solo degli altri N thread
secondo uno schema circolare.
Ciascun thread T_i a sua volta, per ogni stringa letta dal main thread e resa a lui disponibile, 
dovra' scriverla su una nuova linea del file F_i.
L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra' 
riversare su standard-output e su un apposito file chiamato "output-file" il 
contenuto di tutti i file F_i gestiti dall'applicazione 
ricostruendo esattamente la stessa sequenza di stringhe (ciascuna riportata su 
una linea diversa) che era stata immessa tramite lo standard-input.

In caso non vi sia immissione di dati sullo standard-input, l'applicazione dovra' utilizzare 
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

int *input_file_array;
char **input_string_array;
size_t no_files;

typedef struct {
    int thread_no;
    int fd;
} thread_args_t;

typedef thread_args_t *thread_args_ptr_t;

typedef int semaphore;
semaphore sem_go_ahead;
semaphore sem_is_ready;

void *thread_routine(void *arg) {
    thread_args_ptr_t my_arg = (thread_args_ptr_t)arg;
    int thread_no = my_arg->thread_no;
    int fd = my_arg->fd;
    free(my_arg);

    struct sembuf sops;

#ifdef DEBUG
    printf("Thread no. %d opens file descriptor %d\n", thread_no, fd);
    fflush(stdout);
#endif

    sigset_t my_sigset;
    sigfillset(&my_sigset);
    if (sigprocmask(SIG_BLOCK, &my_sigset, NULL) == -1) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    while (1) {
        sops.sem_num = thread_no;
        sops.sem_op = -1;
        
        if (semop(sem_is_ready, &sops, 1) == -1) {
            perror("semop");
            exit(EXIT_FAILURE);
        }

        if (dprintf(fd, "%s\n", *(input_string_array + thread_no)) < 0) {
            perror("dprintf");
            exit(EXIT_FAILURE);
        }

    #ifdef DEBUG
        printf("string %s printed correctly onto file by thread no. %d\n", *(input_string_array + thread_no), thread_no);
    #endif

        sops.sem_num = thread_no;
        sops.sem_op = 1;
        
        if (semop(sem_go_ahead, &sops, 1) == -1) {
            perror("semop");
            exit(EXIT_FAILURE);
        }
    }

    return NULL;
}

void signal_routine(int sig_no) {
    (void)sig_no;

    for (size_t index = 0; index < no_files; index++) {
        lseek(*(input_file_array + index), 0, SEEK_SET);
    }

    FILE **file_streams = calloc(no_files, sizeof(FILE *));
    if (file_streams == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    for (size_t index = 0; index < no_files; index++) {
        if ((*(file_streams + index) = fdopen(*(input_file_array + index), "r+")) == NULL) {
            perror("fdopen");
            exit(EXIT_FAILURE);
        }
    }

    int output_fd;
    if ((output_fd = open("output-file", O_TRUNC | O_CREAT | O_WRONLY, 0666)) == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    char new_string[BUFSIZ];
    size_t no_fails = 0;

    while (1) {
        for (size_t index = 0; index < no_files; index++) {
            errno = 0;
            if (fgets(new_string, BUFSIZ, file_streams[index]) == NULL) {
                if (errno != 0) {
                    perror("fgets");
                    exit(EXIT_FAILURE);
                }
                no_fails++;
                continue;
            }
            dprintf(output_fd, "%s", new_string);
        }

        if (no_fails == no_files) {
            break;
        }
        no_fails = 0;
    }

    close(output_fd);
    free(file_streams);

    puts("--- OVERALL OUTPUT ---");
    system("cat output-file");
    puts("--- OUTPUT PRINTED ---");
}

void free_all_resources(int sig_no) {
	(void)sig_no;
	
	for (size_t i = 0; i < no_files; ++i) {
		semctl(sem_go_ahead, i, IPC_RMID);
		semctl(sem_is_ready, i, IPC_RMID);
	}

#ifdef DEBUG
	puts("The free of all resources has been completed.");
	puts("Check this using \"ipcs -s\" command.");
#endif

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename-1> ... <filename-n>\n", *argv);
        exit(EXIT_FAILURE);
    }

    input_file_array = calloc(argc - 1, sizeof(*input_file_array));
    if (input_file_array == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    input_string_array = calloc(argc - 1, sizeof(*input_string_array));
    if (input_string_array == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    no_files = argc - 1;

    if ((sem_go_ahead = semget(IPC_PRIVATE, argc - 1, 0660)) == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    if ((sem_is_ready = semget(IPC_PRIVATE, argc - 1, 0660)) == -1) {
        perror("semget");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, signal_routine);
    signal(SIGQUIT, free_all_resources);

    pthread_t new_thread;
    for (size_t index = 1; index < (size_t)argc; ++index) {
        if (semctl(sem_go_ahead, index - 1, SETVAL, 1) == -1) {
            perror("semctl");
            exit(EXIT_FAILURE);
        }

        if (semctl(sem_is_ready, index - 1, SETVAL, 0) == -1) {
            perror("semctl");
            exit(EXIT_FAILURE);
        }

        thread_args_ptr_t new_arg = calloc(1, sizeof(thread_args_t));
        if (new_arg == NULL) {
            perror("calloc");
            exit(EXIT_FAILURE);
        }
        
        new_arg->thread_no = (int)(index - 1);
        if ((new_arg->fd = open(*(argv + index), O_CREAT | O_TRUNC | O_RDWR, 0660)) == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        *(input_file_array + index - 1) = new_arg->fd;

        if (pthread_create(&new_thread, NULL, &thread_routine, new_arg) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    char buffer[BUFSIZ];
    // input_buffer[BUFSIZ - 1] = '\0';

    struct sembuf sops;
    
    size_t cycle_index = 0;

    while (1) {

#ifdef DEBUG
        printf("Cycle no. %d\n", (int)cycle_index);
#endif

        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            buffer[strcspn(buffer, "\n")] = '\0';
        }

        if (strlen(buffer) >= sizeof(buffer)) {
            fprintf(stderr, "The string you provided is too large! Please provide a shorter one.\n");
            continue;
        }

        sops.sem_num = (int)cycle_index;
        sops.sem_op = -1;
    
    try_again_op1:
        if (semop(sem_go_ahead, &sops, 1) == -1) {
            if (errno == EINTR) {
                goto try_again_op1;
            }
            perror("semop");
            exit(EXIT_FAILURE);
        }

        if (*(input_string_array + cycle_index) != NULL) {
            free(*(input_string_array + cycle_index));
        }

        if ((*(input_string_array + cycle_index) = strdup(buffer)) == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }

        sops.sem_num = (int)cycle_index;
        sops.sem_op = 1;

    try_again_op2:
        if (semop(sem_is_ready, &sops, 1) == -1) {
            if (errno == EINTR) {
                goto try_again_op2;
            }
            perror("semop");
            exit(EXIT_FAILURE);
        }

        cycle_index = (cycle_index + 1) % (argc - 1);
    }

    return 0;
}