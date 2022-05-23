/// @file client.c
/// @brief Contiene l'implementazione del client.
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "err_exit.h"
#include "defines.h"


int fifo1_fd = -1;

char pathDirectory[250];
int numOfFiles = 0;
off_t size = 4096;

size_t append2Path(char *directory) {
	size_t lastPathEnd = strlen(pathDirectory);
	// extends current seachPath: seachPath + / + directory
	strcat(strcat(&pathDirectory[lastPathEnd], "/"), directory);
	return lastPathEnd;
}

int checkFileName(char *filename1, char *filename2) {
	return (filename1 == NULL || filename2 == NULL) ? 0
		: strncmp(filename1, filename2, 7) == 0;
}

// The signal handler that will be used when the signal SIGUSR1
// is delivered to the process
void sigHandlerTerm(int sig) {
	if (sig == SIGUSR1) {
		printf("Fine Del Programma\n");
		exit(0);
	}
}

// The signal handler that will be used when the signal SIGINT
// is delivered to the process
void sigHandlerStart(int sig) {
	if (sig == SIGINT) {
		printf("Inizio Del Programma\n");
		// set of signals (N.B. it is not initialized!)
		sigset_t mySet;
		// initialize mySet to contain all signals
		sigfillset(&mySet);
		// blocking all signals 
		sigprocmask(SIG_SETMASK, &mySet, NULL);
		// open the Directory
		DIR *dirp = opendir(pathDirectory);
		if (dirp == NULL) {
			printf("non la esiste");
			exit(0);
		}
		char *username = getenv("USER");
 		if (username == NULL)
 			username = "unknown";

 		char buffer[BUFFER_SZ];
 		if (getcwd(buffer, sizeof(buffer)) == NULL) {
 			printf("getcwd failed\n");
 			exit(1);
 		}

 		printf("Ciao %s, ora inizio l’invio dei file contenuti in %s \n", username, buffer);
		
    struct dirent *dentry;

		while ((dentry = readdir(dirp)) != NULL) {
			size_t lastPath = append2Path(dentry->d_name);

			struct stat statbuf;
			if (stat(pathDirectory, &statbuf) == -1)
				ErrExit("errore path");
			if (statbuf.st_size <= size &&           checkFileName(dentry->d_name, "sendme_")) {
				numOfFiles++;
			}
			pathDirectory[lastPath] = '\0';

		}
		printf("numero files: %d \n", numOfFiles);
		closedir(dirp);
		exit(0);
	}


}





int main(int argc, char * argv[]) {
	if (argc != 2) {
		printf("numero di argomenti in input errato\n");
		return 1;
	}
	strcat(pathDirectory, argv[1]);
	// set of signals (N.B. it is not initialized!)
	sigset_t mySet;
	// initialize mySet to contain all signals
	sigfillset(&mySet);
	// remove SIGINT from mySet
	sigdelset(&mySet, SIGINT);
	// remove SIGINT from mySet
	sigdelset(&mySet, SIGUSR1);
	// blocking all signals but SIGINT and SIGUSR1
	sigprocmask(SIG_SETMASK, &mySet, NULL);

	// set the function sigHandler as handler for the signal SIGINT
	if (signal(SIGUSR1, sigHandlerTerm) == SIG_ERR)
		ErrExit("change signal handlerTerm failed");
	// set the function sigHandler as handler for the signal SIGINT
	if (signal(SIGINT, sigHandlerStart) == SIG_ERR)
		ErrExit("change signal handlerStart failed");


	fifo1_fd = open(FIFO1_PATH, O_WRONLY);

	// Send number of files through FIFO1
    char * n_string = int_to_string(numOfFiles);
    msg_t n_msg = {.mtype = N_FILES, .sender_pid = getpid()};
    strcpy(n_msg.msg_body, n_string);
    free(n_string); 

    if (write(fifo1_fd, &n_msg, sizeof(n_msg)) == -1){
		ErrExit("Write FIFO1 failed");
	}
	
	// infinite loop
	while (1) {

	}

	return 0;
}
