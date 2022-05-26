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
#include "semaphore.h"
#include "shared_memory.h"
#include <math.h>

#include "err_exit.h"
#include "defines.h"


int fifo1_fd = -1;
int semid = -1;
int semidStart = -1;
int shmid = -1;
struct dirent *dentry;
pid_t pid;

//Shared memory pointer
msg_t * shm_ptr = NULL;

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

int getDimDivFour(int n) {
	double d= n / 4.0;
	return ceil(d);
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
		semid = createSemaphores(SEM_KEY, 2);
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



		while ((dentry = readdir(dirp)) != NULL) {
			size_t lastPath = append2Path(dentry->d_name);

			struct stat statbuf;
			if (stat(pathDirectory, &statbuf) == -1)
				ErrExit("errore path");
			if (statbuf.st_size <= size && checkFileName(dentry->d_name, "sendme_")) {
				numOfFiles++;
			}
			pathDirectory[lastPath] = '\0';

		}
		printf("numero files: %d \n", numOfFiles);
		semidStart = createSemaphores(SEM_KEY_START, 1);
		semSetVal(semidStart, 0, 0);

		fifo1_fd = open(FIFO1_PATH, O_WRONLY);
		// Send number of files through FIFO1
		char * n_string = int_to_string(numOfFiles);
		msg_t n_msg = { .mtype = N_FILES,.sender_pid = getpid() };
		strcpy(n_msg.msg_body, n_string);
		free(n_string);

		if (write(fifo1_fd, &n_msg, sizeof(n_msg)) == -1) {
			ErrExit("Write FIFO1 failed");
		}
		semOp(semid, 0, 1);
		semOp(semid, 1, -1);
		shmid = alloc_shared_memory(SHM_KEY, IPC_MAX_MSG * sizeof(msg_t));
		shm_ptr = (msg_t *)attach_shared_memory(shmid, IPC_CREAT | S_IRUSR | S_IWUSR);
		if (strcmp(shm_ptr[0].msg_body, "OK") == 0)
			printf("Messaggio ricevuto\n");
		else
			printf("messaggio sbagliato");
		closedir(dirp);
		dirp = opendir(pathDirectory);
		if (dirp == NULL) {
			printf("non la esiste");
			exit(0);
		}
		while ((dentry = readdir(dirp)) != NULL) {


			struct stat statbuf;
			if (stat(pathDirectory, &statbuf) == -1)
				ErrExit("errore path");
			if (statbuf.st_size <= size && checkFileName(dentry->d_name, "sendme_")) {
				//printf("qua printo il file %s \n", dentry->d_name);
				// generate a subprocess
				pid = fork();
				if (pid == -1)
					printf("child  not created!");
				else if (pid == 0) {
					semOp(semidStart, 0, -1);
					printf("PID: %d , PPID: %d.\n",
						getpid(), getppid());
					append2Path(dentry->d_name);
					
					int file = open(pathDirectory, O_RDONLY);
					if (file == -1) {
						printf("File does not exist\n");
					}
					ssize_t bR = 0;
					char bufferOfFile[statbuf.st_size + 1];
					bR = read(file, bufferOfFile, statbuf.st_size);
					int partOfbR = getDimDivFour(bR);
					char bufferOfFile1[partOfbR + 1];
					char bufferOfFile2[partOfbR + 1];
					char bufferOfFile3[partOfbR + 1];
					char bufferOfFile4[partOfbR + 1];
					int i1, i2, i3, i4;
					for (i1 = 0; i1 < partOfbR; i1++) {
						bufferOfFile1[i1] = bufferOfFile[i1];
					}
					bufferOfFile1[i1 + 1] = '\0';
					printf("prima parte file: %s \n", bufferOfFile1);
					//fifo 1

					for (i2 = 0; i2 < partOfbR; i2++) {
						bufferOfFile2[i2] = bufferOfFile[i2+i1];
					}
					bufferOfFile2[i2 + 1] = '\0';
					printf("seconda parte file: %s \n", bufferOfFile2);

					for (i3 = 0; i3 < partOfbR ; i3++) {
						bufferOfFile3[i3] = bufferOfFile[i3+i2+i1];
					}
					bufferOfFile3[i3 + 1] = '\0';
					printf("terza parte file: %s \n", bufferOfFile3);

					for (i4 = 0; i4 < bR-(3* partOfbR); i4++) {
						bufferOfFile4[i4] = bufferOfFile[i4+i3+i2+i1];
					}
					bufferOfFile3[i4 + 1] = '\0';
					printf("quarta parte file: %s \n", bufferOfFile4);
						

					// close the file descriptor
					close(file);
					exit(0);
				}



				
				

				//il padre sblocca il semaforo in modo che i figli inviino
			}
		}

		if (pid != 0) {
			printf("PArent PID: %d , NOnno PID: %d.\n",
				getpid(), getppid());
			semOp(semidStart, 0, numOfFiles);

		}


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




	// infinite loop
	while (true) {

	}

	return 0;
}

