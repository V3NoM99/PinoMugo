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
#include <sys/msg.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "semaphore.h"
#include "shared_memory.h"
#include "fifo.h"
#include <math.h>

#include "err_exit.h"
#include "defines.h"

int fd_fifo1 = -1; // fileDescriptor FIFO1
int fd_fifo2 = -1; // fileDescriptor FIFO2
int msqid = -1;	   // id MessageQueue
int semid = -1;	   // id Semaforo
int shmid = -1;	   // id SharedMemory
int shm_check_id = -1;
msg_t *shm_ptr = NULL; // puntatore SharedMemory
int *shm_check_ptr = NULL;
pid_t pid;
int numOfFiles;
char pathDirectory[250];
off_t size = 4096;

// funzione che va ad estendere il path corrente con ciò che viene passato in input
size_t append2Path(char *directory)
{
	size_t lastPathEnd = strlen(pathDirectory);
	// extends current seachPath: seachPath + / + directory
	strcat(strcat(&pathDirectory[lastPathEnd], "/"), directory);
	return lastPathEnd;
}

// controlla se due file iniziano con gli stessi 7 primi caratteri
int checkFileName(char *filename1, char *filename2)
{
	return (filename1 == NULL || filename2 == NULL) ? 0
													: strncmp(filename1, filename2, 7) == 0;
}

// divide un numero per 4 approssimando per eccesso
int getDimDivFour(int n)
{
	double d = n / 4.0;
	return ceil(d);
}

// Il signal handler che verà usato quando il segnale SIGUSR1 arriverà al processo
void sigHandlerTerm(int sig)
{
	if (sig == SIGUSR1)
	{
		printf("Fine Del Programma\n");
		exit(0);
	}
}

// va ad aprire tutte le ipc necessarie
void InitializeIpc()
{
	if (semid == -1)
		semid = createSemaphores(SEM_KEY, 10);
	if (semid == -1)
		ErrExit("Creation Sempaphore failed");
	if (fd_fifo1 == -1)
		fd_fifo1 = open(FIFO1_PATH, O_WRONLY);
	if (fd_fifo1 == -1)
		ErrExit("Creation FIFO1 failed");
	if (fd_fifo2 == -1)
		fd_fifo2 = open(FIFO2_PATH, O_WRONLY);
	if (fd_fifo2 == -1)
		ErrExit("Creation FIFO2 failed");
	if (msqid == -1)
		msqid = msgget(MSQ_KEY, IPC_CREAT | S_IRUSR | S_IWUSR);
	if (msqid == -1)
		ErrExit("Creation Msqid Failed");
	if (shmid == -1)
		shmid = alloc_shared_memory(SHM_KEY, IPC_MAX_MSG * 1024);
	if (shmid == -1)
		ErrExit("Error Allocation SHDMEM");
	if (shm_ptr == NULL)
		shm_ptr = (msg_t *)attach_shared_memory(shmid, IPC_CREAT | S_IRUSR | S_IWUSR);
	if (shm_check_id == -1)
		shm_check_id = alloc_shared_memory(SHM_CHECK_KEY, IPC_MAX_MSG * sizeof(int));
	if (shm_check_id == -1)
		ErrExit("Error Allocation SHDMEM CHECKID");
	if (shm_check_ptr == NULL)
		shm_check_ptr = (int *)attach_shared_memory(shm_check_id, S_IRUSR | S_IWUSR);
}

// codice eseguito dal figlio
void execSon(struct stat statbuf)
{
	int processId = getpid();
	int file = open(pathDirectory, O_RDONLY);
	if (file == -1)
	{
		printf("File does not exist\n");
	}
	ssize_t bR = 0;
	char bufferOfFile[statbuf.st_size];
	bR = read(file, bufferOfFile, statbuf.st_size);
	bufferOfFile[statbuf.st_size] = '\0';
	int partOfbR = getDimDivFour(bR);
	char bufferOfFile1[partOfbR];
	char bufferOfFile2[partOfbR];
	char bufferOfFile3[partOfbR];
	char bufferOfFile4[partOfbR];
	int i1, i2, i3, i4;
	bool part1 = false,
		 part2 = false,
		 part3 = false,
		 part4 = false;

	// PREPARO BUFFER FIFO1
	for (i1 = 0; i1 < partOfbR; i1++)
	{
		if (bufferOfFile[i1] != 0)
			bufferOfFile1[i1] = bufferOfFile[i1];
		else
			bufferOfFile1[i1] = ' ';
	}
	bufferOfFile1[i1] = '\0';
	msg_t fifo1_msg = {.mtype = 2, .sender_pid = processId};
	strcpy(fifo1_msg.msg_body, bufferOfFile1);
	strcpy(fifo1_msg.file_path, pathDirectory);
	// PREPARO BUFFER FIFO2
	for (i2 = 0; i2 < partOfbR; i2++)
	{
		if (bufferOfFile[i2 + i1] != 0)
			bufferOfFile2[i2] = bufferOfFile[i2 + i1];
		else
			bufferOfFile2[i2] = ' ';
	}
	bufferOfFile2[i2] = '\0';
	msg_t fifo2_msg = {.mtype = 2, .sender_pid = processId};
	strcpy(fifo2_msg.msg_body, bufferOfFile2);
	strcpy(fifo2_msg.file_path, pathDirectory);
	// PREPARO BUFFER MsgQueue
	for (i3 = 0; i3 < partOfbR; i3++)
	{
		if (bufferOfFile[i3 + i2 + i1] != 0)
			bufferOfFile3[i3] = bufferOfFile[i3 + i2 + i1];
		else
			bufferOfFile3[i3] = ' ';
	}
	bufferOfFile3[i3] = '\0';
	msg_t msgQueue_msg = {.mtype = 3, .sender_pid = processId};
	strcpy(msgQueue_msg.msg_body, bufferOfFile3);
	strcpy(msgQueue_msg.file_path, pathDirectory);
	// PREPARO BUFFER  ShdMem
	for (i4 = 0; i4 < partOfbR; i4++)
	{
		if (bufferOfFile[i4 + i3 + i2 + i1] != 0)
			bufferOfFile4[i4] = bufferOfFile[i4 + i3 + i2 + i1];
		else
			bufferOfFile4[i4] = ' ';
	}
	bufferOfFile4[i4] = '\0';
	msg_t shdmem_msg = {.mtype = 4, .sender_pid = processId};
	strcpy(shdmem_msg.msg_body, bufferOfFile4);
	strcpy(shdmem_msg.file_path, pathDirectory);

	// close the file descriptor
	close(file);

	semOp(semid, SEMNUMFILES, -1);
	semOp(semid, SEMNUMFILES, 0);

	// finchè non invia tutte le parti
	while (part1 == false || part2 == false || part3 == false || part4 == false)
	{

		// invio parte1 su fifo
		if (part1 == false)
		{
			if (semOpNoWait(semid, FIFO1LIMIT, -1) == 0)
			{
				if (write(fd_fifo1, &fifo1_msg, sizeof(fifo1_msg)) != -1)
					part1 = true;
				else
					semOp(semid, FIFO1LIMIT, 1);
			}
		}
		// invio parte2 su fifo
		if (part2 == false)
		{
			if (semOpNoWait(semid, FIFO2LIMIT, -1) == 0)
			{
				if (write(fd_fifo2, &fifo2_msg, sizeof(fifo2_msg)) != -1)
					part2 = true;
				else
					semOp(semid, FIFO2LIMIT, 1);
			}
		}

		// invio parte 3 su messageQuee
		if (part3 == false)
		{
			if (semOpNoWait(semid, MSQLIMIT, -1) == 0)
			{
				if (msgsnd(msqid, &msgQueue_msg, sizeof(struct msg_t) - sizeof(long), IPC_NOWAIT) != -1)
					part3 = true;
				else
					semOp(semid, MSQLIMIT, 1);
			}
		}

		// invio parte 4 su shdMem
		if (part4 == false)
		{
			if (semOpNoWait(semid, SHMSEM, -1) == 0)
			{
				for (int i = 0; i < IPC_MAX_MSG; i++)
				{
					if (shm_check_ptr[i] == 0)
					{
						shm_check_ptr[i] = 1;
						shm_ptr[i] = shdmem_msg;
						part4 = true;
						break;
					}
				}
				semOp(semid, SHMSEM, 1);
			}
		}
	}
	exit(0);
}

// genera i processi figlio ed ogni figlio eseguirà la sua parte
void GenerateSons()
{
	// apre la Directory
	DIR *dirp = opendir(pathDirectory);
	if (dirp == NULL)
		ErrExit("directory inesistente");

	// legge la directory corrente fin tanto che trova qualcosa
	struct dirent *dentry;
	while ((dentry = readdir(dirp)) != NULL)
	{
		if (strcmp(dentry->d_name, ".") == 0 ||
			strcmp(dentry->d_name, "..") == 0)
		{
			continue;
		}
		if (dentry->d_type == DT_REG)
		{
			// appende il nome file al path corrente
			size_t lastPath = append2Path(dentry->d_name);

			// copia il pathDirectory nella struct stat
			struct stat statbuf;
			if (stat(pathDirectory, &statbuf) == -1)
				ErrExit("errore path");
			if (statbuf.st_size <= size && checkFileName(dentry->d_name, "sendme_"))
			{
				// genero processo figlio
				pid = fork();
				if (pid == -1)
					printf("child  not created!");
				else if (pid == 0)
				{
					execSon(statbuf);
				}
			}
			// rimuove il nome file al path corrente
			pathDirectory[lastPath] = '\0';
		}
		else if (dentry->d_type == DT_DIR)
		{
			// appende il nome directory al path corrente
			size_t lastPath = append2Path(dentry->d_name);

			// chiamata ricorsiva della funzione stessa
			GenerateSons();

			// rimuove il nome directory al path corrente
			pathDirectory[lastPath] = '\0';
		}
	}
}

// cerca il numero di files regolari
// contenuti nel path passato in input e nelle sue sotto directory
void search()
{
	// apre la Directory
	DIR *dirp = opendir(pathDirectory);
	if (dirp == NULL)
		ErrExit("directory inesistente");

	// legge la directory corrente fin tanto che trova qualcosa
	struct dirent *dentry;
	while ((dentry = readdir(dirp)) != NULL)
	{
		if (strcmp(dentry->d_name, ".") == 0 ||
			strcmp(dentry->d_name, "..") == 0)
		{
			continue;
		}
		if (dentry->d_type == DT_REG)
		{
			// appende il nome file al path corrente
			size_t lastPath = append2Path(dentry->d_name);

			// copia il pathDirectory nella struct stat
			struct stat statbuf;
			if (stat(pathDirectory, &statbuf) == -1)
				ErrExit("errore path");

			// controlla che il file sia regolare(<4kb start with:sendme_)
			if (statbuf.st_size <= size && checkFileName(dentry->d_name, "sendme_"))
				numOfFiles++;

			// rimuove il nome file al path corrente
			pathDirectory[lastPath] = '\0';
		}
		else if (dentry->d_type == DT_DIR)
		{
			// appende il nome directory al path corrente
			size_t lastPath = append2Path(dentry->d_name);

			// chiamata ricorsiva della funzione stessa
			search();

			// rimuove il nome directory al path corrente
			pathDirectory[lastPath] = '\0';
		}
	}

	// chiude la directory aperta precedentemente
	closedir(dirp);
}

// Il signal handler che verà usato quando il segnale SIGINT arriverà al processo
void sigHandlerStart(int sig)
{
	if (sig == SIGINT)
	{
		printf("Inizio Del Programma\n");

		// resetto numero di files
		numOfFiles = 0;

		// set di segnali (N.B. non è inizializzato!)
		sigset_t mySet;

		// inizializza mySet per contenere tutti i signals
		sigfillset(&mySet);

		// blocca tutti i segnali
		sigprocmask(SIG_SETMASK, &mySet, NULL);

		// ottengo il nome dell'utente che esegue il programma
		char *username = getenv("USER");
		if (username == NULL)
			username = "unknown";

		// ottengo il path della corrent working directory
		char buffer[BUFFER_SZ];
		if (getcwd(buffer, sizeof(buffer)) == NULL)
		{
			printf("getcwd failed\n");
			exit(1);
		}

		// saluto l'utente e mi preparo ad inviare il numero di files
		printf("Ciao %s, ora inizio l’invio dei file contenuti in %s \n", username, buffer);

		// va a cercare nella directory passata in input il numero di files regolari
		search();

		// inizializzo il  semaforo contenente numero di files
		semSetVal(semid, 5, numOfFiles); // SEMNUMFILES

		// Invia il  numero di files tramite FIFO1
		char *n_string = int_to_string(numOfFiles);
		msg_t n_msg = {.mtype = N_FILES, .sender_pid = getpid()};
		strcpy(n_msg.msg_body, n_string);
		free(n_string);
		if (write(fd_fifo1, &n_msg, sizeof(n_msg)) == -1)
		{
			ErrExit("Write FIFO1 failed");
		}
		semOp(semid, FIFO1SEM, 1);		// CHIEDERE COME S CHIAMA
		semOp(semid, SHMOK, -1); 

		// controllo che il messaggio che arriva sia "OK" e di tipo
		if (strcmp(shm_ptr[0].msg_body, "OK") == 0 && shm_ptr[0].mtype == N_FILES)
		{
			// libero la shared memory dal messaggio
			shm_check_ptr[0] = 0;

			// genera figli
			GenerateSons();

			// CODICE ESEGUITO DAL PADRE CLIENT_0
			if (pid != 0)
			{

				semOp(semid, BLOCKFINISHED, -1); // BLOCK FINISCHED
				msg_t term;
				if (msgrcv(msqid, &term, sizeof(struct msg_t) - sizeof(long), FINISHED, 0) != -1)
				{
					sigfillset(&mySet);
					sigprocmask(SIG_SETMASK, &mySet, NULL);
				}
			}
		}
	}
}

// FUNZIONE MAIN
int main(int argc, char *argv[])
{
	// controllo che il numero di argomenti passati in input sia solo 1
	if (argc != 2)
	{
		printf("numero di argomenti in input errato\n");
		return 1;
	}

	// funzione che apre tutte le Ipc
	InitializeIpc();

	// scrie su pathDirectory ciò che passo come unico argomento al client
	strcat(pathDirectory, argv[1]);

	// set di segnali (N.B. non è inizializzato!)
	sigset_t mySet;

	// inizializza mySet per contenere tutti i segnali
	sigfillset(&mySet);

	// rimuove SIGINT da mySet
	sigdelset(&mySet, SIGINT);

	// rimuove SIGUSR1 da mySet
	sigdelset(&mySet, SIGUSR1);

	// blocca tutti i signali tranne SIGINT e SIGUSR1
	sigprocmask(SIG_SETMASK, &mySet, NULL);

	/// setta la funzione sigHandlerTerm come handler per il segnale SIGUSR1
	if (signal(SIGUSR1, sigHandlerTerm) == SIG_ERR)
		ErrExit("change signal handlerTerm failed");

	// setta la funzione sigHandlerStart come handler per il segnale SIGINT
	if (signal(SIGINT, sigHandlerStart) == SIG_ERR)
		ErrExit("change signal handlerStart failed");

	// loop infinito che tiene in esecuzione il client
	while (true)
	{
	}

	return 0;
}
