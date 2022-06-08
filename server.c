/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "defines.h"
#include "err_exit.h"
#include "fifo.h"
#include "semaphore.h"
#include "shared_memory.h"

// Inizializzazione file descriptors e identifiers
int fd_fifo1 = -1; // FD FIFO1
int fd_fifo2 = -1; // FD FIFO2

int msqid = -1; // ID MESSAGE QUEUE
int shmid = -1; // ID SHARED MEMORY
int semid = -1; // ID SEMAPHORES SET
int shm_check_id = -1;
// Puntatori alle due Shared Memory
msg_t *shm_ptr = NULL;
int *shm_check_ptr = NULL; // ShM check interi che indicano il posto in cui ho
                           // spazio libero per il messaggio

//---------------------------------------------------
// FUNZIONI INTERNE AL SERVER

int create_sem_set(key_t semkey) {
  // Create a semaphore set with 10 semaphores
  semid = createSemaphores(SEM_KEY, 10);
  unsigned short semValues[] = {1, 1, 1,           0,           0,
                                0, 1, IPC_MAX_MSG, IPC_MAX_MSG, IPC_MAX_MSG};
  semSetAll(semid, semValues);

  return semid;
}

/**
 * @brief Handler for server termination
 *
 * @param sig signal
 */
void SignalHandlerTerm(int sig) {

  	// Chiude FIFO1
  	if (fd_fifo1 != -1) {
		if (close(fd_fifo1) == -1) {
      		ErrExit("<SERVER> close FIFO1 failed");
    }

    if (unlink(FIFO1_PATH) == -1) {
     	 ErrExit("<SERVER> unlink FIFO1 failed");
    }
  	}
  	// Chiude FIFO2
	if (fd_fifo2 != -1) {
	  	if (close(fd_fifo2) == -1) {
      		ErrExit("<SERVER> close FIFO2 failed");
    	}
		if (unlink(FIFO2_PATH) == -1) {
			ErrExit("<SERVER> unlink FIFO2 failed");
    	}
  	}

  	// Chiude set dei semafori
  	if (semid != -1)
		semDelete(semid);

  	// chiude  e dealloca la shared memory
  	if (shm_ptr != NULL)
    	free_shared_memory(shm_ptr);
  	if (shmid != -1)
    	remove_shared_memory(shmid);

  	// chiude e dealloca la shared memory check
  	if (shm_check_ptr != NULL)
    	free_shared_memory(shm_check_ptr);
  	if (shm_check_id != -1)
    	remove_shared_memory(shm_check_id);

  	// chiude la message queue
  	if (msqid != -1) {
    	if (msgctl(msqid, IPC_RMID, NULL) == -1) {
      	ErrExit("<SERVER> msgctl failed");
    	}
  	}

  	exit(0);
}

//---------------------------------------------------
// MAIN

int main(int argc, char *argv[]) {

  	// settaggio del signal handler per chiudere le IPCs e terminare server
  	if (signal(SIGINT, SignalHandlerTerm) == SIG_ERR) {
   		ErrExit("<SERVER> Change of signal handler failed");
  	}

  	// MsgQueue
  	msqid = msgget(MSQ_KEY, IPC_CREAT | S_IRUSR | S_IWUSR);

  	// Settaggio del limite della MSQ

  	struct msqid_ds ds;
  	if (msgctl(msqid, IPC_STAT, &ds) == -1)
    	ErrExit("msgctl STAT");
  	ds.msg_qbytes = sizeof(msg_t) * IPC_MAX_MSG;
  	if (msgctl(msqid, IPC_SET, &ds) == -1) {
    	if (errno != EPERM) {
      		ErrExit("msgctl SET");
    	} else {
      		printf("<SERVER> Not enough permissions to set new MSQ limit, enable "
             "CAP_SYS_RESOURCE for server\n");
    	}
  	}

  	//  Creazione della SharedMemory
  	shmid = alloc_shared_memory(SHM_KEY, IPC_MAX_MSG * sizeof(msg_t));
  	shm_ptr = (msg_t *)attach_shared_memory(shmid, IPC_CREAT | S_IRUSR | S_IWUSR);
  	if (shm_check_id == -1)
    	shm_check_id = alloc_shared_memory(SHM_CHECK_KEY, IPC_MAX_MSG * sizeof(int));
  	if (shm_check_id == -1)
    	ErrExit("<SERVER> Error Allocation SHDMEM CHECKID");
  	if (shm_check_ptr == NULL)
    	shm_check_ptr =(int *)attach_shared_memory(shm_check_id, S_IRUSR | S_IWUSR);

  	// Creazione del SemaphoreSet
  	semid = create_sem_set(SEM_KEY);

  	// Creazione della FIFO2
  	make_fifo(FIFO2_PATH);
  	fd_fifo2 = open(FIFO2_PATH, O_RDONLY | O_NONBLOCK);
  	// Creazione della FIFO1
  	fd_fifo1 = create_fifo(FIFO1_PATH, 'r'); // Server wants to read

  	while (true) {
    	semOp(semid, FIFO1SEM, -1);
    	// Il Server attende l'arrivo del numero di files
    	msg_t n_files;
    	if (read(fd_fifo1, &n_files, sizeof(msg_t)) == -1)
      		ErrExit("Read failed");
    	if (strcmp((n_files.msg_body), "0") != 0) {
      	int n = 0;
      	if ((n = atoi(n_files.msg_body)) == 0)
        	ErrExit("Atoi failed!");
      	printf("number of files: %d \n", n);

      	// Scrive il messaggio OK nella shared memory
      	msg_t received_msg = {
          	.msg_body = "OK", 
			.mtype = N_FILES, 
			.sender_pid = getpid()
			};

      	shm_ptr[0] = received_msg;
	
      	semOp(semid, SHMOK, 1); //libera la lettura su shm al client

      	// Prima di iterare sui canali rendo le fifo non bloccanti
      	printf("<SERVER> making FIFOs NON BLOCKING\n");
      	semOp(semid, FIFO1SEM, -1);
      	semOp(semid, FIFO2SEM, -1);
      	blockFifo(fd_fifo1, 0); // 0 = false
      	blockFifo(fd_fifo2, 0);
      	semOp(semid, FIFO1SEM, 1);
      	semOp(semid, FIFO2SEM, 1);
		
      	printf("<SERVER> FIFOs are NON BLOCKING\n");

      	// Ciclically waiting for messages
      	int total_number_parts = n * 4; // total number of parts I have to receive
      	int received_parts = 0;
      	char *path = "_out";

      	while (received_parts < total_number_parts) {

        	// Save sender PID, message body of the message part, and file path
        	msg_t channel1, channel2, channel3;

        	// LEGGO DA FIFO 1 la prima parte del file
        	if (semOpNoWait(semid, MUTEXFILE, -1) == 0) {
          		if (read(fd_fifo1, &channel1, sizeof(channel1)) != -1) {
            		char *outputOnFile1 = malloc(2000);
            		char *completepath1 = malloc(259);
            		int l1;
            		sprintf(outputOnFile1,
                    "[Parte1, del file %s spedita dal processo %d tramite "
                    "FIFO1]\n%s\n",
                    channel1.file_path, channel1.sender_pid, channel1.msg_body);
            		l1 = strlen(outputOnFile1);
            		completepath1 = channel1.file_path;
            		strcat(completepath1, path);
            		int fileD =
                			open(completepath1, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR);
            		// movet the current offset to the bottom of destination file
            		if (fileD != -1 && lseek(fileD, 0, SEEK_SET) == -1)
              			ErrExit("lssek failed");
            		if (write(fileD, outputOnFile1, l1) != l1)
              			ErrExit("Errore Scrittura File");
            		received_parts++;
            		close(fileD);
            		semOp(semid, FIFO1LIMIT, 1);
          		}
          	semOp(semid, MUTEXFILE, 1);
        	}

        	// LEGGO DA FIFO 2 la seconda parte del file
        	if (semOpNoWait(semid, MUTEXFILE, -1) == 0) {
          		if (read(fd_fifo2, &channel2, sizeof(channel2)) != -1) {
            	char *outputOnFile2 = malloc(2000);
            	char *completepath2 = malloc(259);
            	int l2;
            	sprintf(outputOnFile2,
                    "\n[Parte2,del file %s spedita dal processo %d tramite "
                    "FIFO2]\n%s\n",
                    channel2.file_path, channel2.sender_pid, channel2.msg_body);
            	l2 = strlen(outputOnFile2);
            	completepath2 = channel2.file_path;
            	strcat(completepath2, path);
            	int fileD =
                	open(completepath2, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR);
            	// movet the current offset to the bottom of destination file
            	if (fileD != -1 && lseek(fileD, l2, SEEK_SET) == -1)
              		ErrExit("lssek failed");
            	if (write(fileD, outputOnFile2, l2) != l2)
              		ErrExit("Errore Scrittura File");
            	received_parts++;
            	close(fileD);
            	semOp(semid, FIFO2LIMIT, 1);
          		}
          		semOp(semid, MUTEXFILE, 1);
        	}

        	// LEGGO DA CODA DI MESSAGGI la terza parte del file
        	if (semOpNoWait(semid, MUTEXFILE, -1) == 0) {
          		if (msgrcv(msqid, &channel3, sizeof(struct msg_t) - sizeof(long),
                     MSQ_PART, IPC_NOWAIT) != -1) {
            	char *outputOnFile3 = malloc(2000);
            	char *completepath3 = malloc(259);
            	int l3;
            	sprintf(outputOnFile3,
                    "\n[Parte3,del file %s spedita dal processo %d tramite "
                    "MsgQueue]\n%s\n",
                    channel3.file_path, channel3.sender_pid, channel3.msg_body);
            	l3 = strlen(outputOnFile3);
            	completepath3 = channel3.file_path;
            	strcat(completepath3, path);
            	int fileD =
                	open(completepath3, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR);
            	// movet the current offset to the bottom of destination file
            	if (fileD != -1 && lseek(fileD, (l3 * 2) - 6, SEEK_SET) == -1)
              	ErrExit("lssek failed");
            	if (write(fileD, outputOnFile3, l3) != l3)
              		ErrExit("Errore Scrittura File");
            	received_parts++;
            	close(fileD);
            	semOp(semid, MSQLIMIT, 1);
          	}
         	semOp(semid, MUTEXFILE, 1);
        	}

        	// LEGGO DA SHDMEM
        	if (semOpNoWait(semid, MUTEXFILE, -1) == 0) {
          		if (semOpNoWait(semid, SHMSEM, -1) == 0) {
            	for (int i = 0; i < IPC_MAX_MSG; i++) {
              	if (shm_check_ptr[i] == 1) {
                	char *outputOnFile4 = malloc(2000);
                	char *completepath4 = malloc(259);
                	int l4;
                	shm_check_ptr[i] = 0;
                	sprintf(outputOnFile4,
                        "\n[Parte 4, del file %s , spedita dal processo %d "
                        "tramite ShdMem]\n%s\n",
                        shm_ptr[i].file_path, shm_ptr[i].sender_pid,
                        shm_ptr[i].msg_body);
                	l4 = strlen(outputOnFile4);
               		completepath4 = shm_ptr[i].file_path;
                	strcat(completepath4, path);
                	int fileD =
                    	open(completepath4, O_CREAT | O_WRONLY, S_IWUSR | S_IRUSR);
                	// movet the current offset to the bottom of destination file
                	if (fileD != -1 && lseek(fileD, (l4 * 3) - 12, SEEK_SET) == -1)
                  		ErrExit("lssek failed");
                	if (write(fileD, outputOnFile4, l4) != l4)
                 		ErrExit("Errore Scrittura File");
                	received_parts++;
                	close(fileD);
              		}
            	}
           	 	semOp(semid, SHMSEM, 1);
          	}
          	semOp(semid, MUTEXFILE, 1);
        	}
      	}
      	// HO RICEVUTO TUTTE LE PARTI, LE DEVO RIUNIRE
      	printf("\nHO RICEVUTO TUTTO\n");

      	// RENDO LE FIFO DI NUOVO BLOCCANTI
      	printf("<SERVER> making FIFOs BLOCKING\n");
      	semOp(semid, FIFO1SEM, -1);
      	semOp(semid, FIFO2SEM, -1);
      	blockFifo(fd_fifo1, 1); // 0 = false
      	blockFifo(fd_fifo2, 1);
      	semOp(semid, FIFO1SEM, 1);
      	semOp(semid, FIFO2SEM, 1);
      	printf("<SERVER> FIFOs are BLOCKING\n");
      	// Wait again for a new number of files to receive

      	// sending message to client: I've ended my work
      	msg_t end_msg = {
          	.msg_body = "FINISHED", 
				.mtype = FINISHED, 
					.sender_pid = getpid()
			};
      	msgsnd(msqid, &end_msg, sizeof(struct msg_t) - sizeof(long), 0);
     	semOp(semid, BLOCKFINISHED, 1);
    	} else {
      	printf("Il numero dei file nella cartella è uguale a 0\n");
      	printf("Riempire la cartella con dei file sendme_\n");
    	}
  	}

  	return 0;
}
