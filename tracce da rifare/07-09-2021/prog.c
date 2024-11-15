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
Scrivere un programma che riceva in input tramite argv[] N coppie di stringhe 
con N maggiore o uguale a 1, in cui la prima stringa della coppia indica il 
nome di un file.
Per ogni coppia di strighe dovra' essere attivato un processo che dovra' creare 
il file associato alla prima delle stringhe della coppia o poi ogni 5 secondi 
dovra' effettuare il controllo su quante volte la seconda delle stringhe della 
coppia compare nel file, riportando il risultato su standard output.
Il main thread del processo originale dovra' leggere lo stream da standard input in 
modo indefinito, e dovra' scrivere i byte letti in uno dei file identificati 
tramite i nomi passati con argv[]. La scelta del file dove scrivere dovra' 
avvenire in modo circolare a partire dal primo file identificato tramite argv[], 
e il cambio del file destinazione dovra' avvenire qualora venga ricevuto il 
segnale SIGINT (o CTRL_C_EVENT nel caso WinAPI).
Se il segnale SIGINT (o CTRL_C_EVENT nel caso WinAPI) colpira' invece uno degli 
altri processi, questo dovra' riportare il contenuto del file che sta gestendo 
su standard output.

In caso non vi sia immissione di dati sullo standard input, l'applicazione 
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

volatile sig_atomic_t i = 1, j = 2, received_sigint = 0;

void signal_handler(int sig) {
	if (sig == SIGINT) {
        received_sigint = 1;
		i += 2;
        j = i + 1;
	}
}

int main(int argc, const char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <string-1> <string-2> ... <string-n>\n", *argv);
		exit(EXIT_FAILURE);
	}

	if (signal(SIGINT, signal_handler) == SIG_ERR) {
		perror("signal");
		exit(EXIT_FAILURE);
	}

	int n = argc - 1; // number of strings
    int fd;
    for (ssize_t k = 1; k < n; k += 2) {
        fd = open(argv[k], O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("fd1");
            exit(EXIT_FAILURE);
        }
        close(fd);
    }

	for (; i < n; i += 2, j = i + 1) {
        pid_t pid = fork();
		if (pid == -1) {
			perror("Fork");
			exit(EXIT_FAILURE);
		} else if (pid == 0) { // child process
			while (1) {
                if (i >= n) { i = 1; j = 2; }
                fd = open(argv[i], O_CREAT | O_RDONLY, 0644);
				if (fd == -1) {
					perror("fd2");
					exit(EXIT_FAILURE);
				}

                // todo: fix this with strtok() !!!
				char buffer[BUFSIZ];
				ssize_t bytes_read;
				int occurrences = 0;
				char *line_start = buffer;
				while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
					for (ssize_t k = 0; k < bytes_read; k++) {
						if (buffer[k] == '\n' || k == bytes_read - 1) {
							ssize_t line_length = &buffer[k] - line_start + 1;
							char line[line_length];
							strncpy(line, line_start, line_length - 1);
							line[line_length - 1] = '\0';

							if (!strcmp(line, argv[j])) {
								occurrences++;
							}

							line_start = &buffer[k + 1];
						}
					}
				}
				printf("Occurrences of '%s' in %s: %d\n", argv[j], argv[i], occurrences);
                close(fd);
				sleep(5);
			}
			
		} else { // parent process
			fd = open(argv[i], O_WRONLY, 0644);
			if (fd == -1) {
				perror("fd3");
				exit(EXIT_FAILURE);
			}

            char buffer[BUFSIZ];
            ssize_t bytes_read;
            while (1) {
                bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer));
                if (bytes_read > 0) {
                    if (received_sigint) {
                        printf("%d %s\n", i, argv[i]);
                        received_sigint = 0;
                        close(fd);
                        if (i >= n) {
                            i = 1;
                        }
                        fd = open(argv[i], O_APPEND | O_WRONLY, 0644);
                        if (fd == -1) {
                            perror("fd4");
                            exit(EXIT_FAILURE);
                        }
                    }
                    write(fd, buffer, bytes_read);
                }
            }
		}
	}

	return 0;
}
