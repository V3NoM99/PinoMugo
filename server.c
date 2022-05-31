/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <fcntl.h>


#include "err_exit.h"
#include "defines.h"
#include "shared_memory.h"
#include "semaphore.h"
#include "fifo.h"

//File descriptors and identifiers inizialization
int fd_fifo1 = -1; // FD FIFO1
int fd_fifo2 = -1; // FD FIFO2

int msqid = -1;    // ID MESSAGE QUEUE
int shmid = -1;    // ID SHARED MEMORY
int semid = -1;    // ID SEMAPHORES SET
int shm_check_id = -1;
//Shared memory pointer
msg_t * shm_ptr = NULL;
int * shm_check_ptr = NULL;
msg_t **matrixFile = NULL;

//---------------------------------------------------
// SERVER INTERNAL FUNCTIONS

int create_sem_set(key_t semkey) {
    // Create a semaphore set with 10 semaphores
	semid = createSemaphores(SEM_KEY, 10);
	unsigned short semValues[] = { 1, 1, 0, 0, 0, 0, 1, IPC_MAX_MSG, IPC_MAX_MSG, IPC_MAX_MSG };
	semSetAll(semid, semValues);

    return semid;
}

/**
 * @brief Handler for server termination
 *
 * @param sig signal
*/
void SignalHandlerTerm(int sig) {

	// close FIFO1
	if (fd_fifo1 != -1) {
		if (close(fd_fifo1) == -1) {
			ErrExit("SERVER: close FIFO1 failed");
		}

		if (unlink(FIFO1_PATH) == -1) {
			ErrExit("SERVER: unlink FIFO1 failed");
		}
	}
	// close FIFO2
	if (fd_fifo2 != -1) {
		if (close(fd_fifo2) == -1) {
			ErrExit("SERVER: close FIFO2 failed");
		}

		if (unlink(FIFO2_PATH) == -1) {
			ErrExit("SERVER: unlink FIFO2 failed");
		}
	}

	// close semaphores
	if (semid != -1)
		semDelete(semid);

	// close amd deallocates shared memory
	if (shm_ptr != NULL)
		free_shared_memory(shm_ptr);
	if (shmid != -1)
		remove_shared_memory(shmid);

	// close amd deallocates shared memory check
	if (shm_check_ptr != NULL)
		free_shared_memory(shm_check_ptr);
	if (shm_check_id != -1)
		remove_shared_memory(shm_check_id);

	// close message queue
	if (msqid != -1) {
		if (msgctl(msqid, IPC_RMID, NULL) == -1) {
			ErrExit("SERVER: msgctl failed");
		}
	}



	exit(0);
}

char * costruisciStringa(msg_t a) {
	char buffer[20]; // serve per convertire il pid
	sprintf(buffer, "%d", a.sender_pid);

	char * stringa = (char *)malloc((strlen(a.msg_body) + strlen(a.file_path) + strlen(buffer) + 61) * sizeof(char));

	strcpy(stringa, "[Parte ");

	switch (a.mtype) {
	case FIFO1_PART:
		strcat(stringa, "1, del file ");
		break;

	case FIFO2_PART:
		strcat(stringa, "2, del file ");
		break;

	case MSQ_PART:
		strcat(stringa, "3, del file ");
		break;

	case SHM_PART:
		strcat(stringa, "4, del file ");
		break;

	default:
		break;
	}

	strcat(stringa, a.file_path);
	strcat(stringa, " spedita dal processo ");
	strcat(stringa, buffer);
	strcat(stringa, " tramite ");

	switch (a.mtype) {
	case FIFO1_PART:
		strcat(stringa, "FIFO1]\n");
		break;
	case FIFO2_PART:
		strcat(stringa, "FIFO2]\n");
		break;
	case MSQ_PART:
		strcat(stringa, "MsgQueue]\n");
		break;
	case SHM_PART:
		strcat(stringa, "ShdMem]\n");
		break;
	default:
		break;
	}

	strcat(stringa, a.msg_body);
	return stringa;
}


void addToMatrix(msg_t a, int righe) {
	bool aggiunto = false;
	for (int i = 0; i < righe && aggiunto == false; i++)
		for (int j = 0; j < 4 && aggiunto == false; j++) {
			if (strcmp(matrixFile[i][j].file_path, a.file_path) == 0) {
				matrixFile[i][a.mtype - 2] = a;
				aggiunto = true;
			}
		}

	for (int i = 0; i < righe && aggiunto == false; i++) {
		//cerco la prima riga vuota
		if (matrixFile[i][0].mtype == INIT && matrixFile[i][1].mtype == INIT && matrixFile[i][2].mtype == INIT && matrixFile[i][3].mtype == INIT) {
			matrixFile[i][a.mtype - 2] = a;
			aggiunto = true;
		}
	}
}


void findAndMakeFullFiles(int righe) {
	for (int i = 0; i < righe; i++) {
		// cerca righe complete
		bool fullLine = true;
		for (int j = 0; j < 4 && fullLine; j++) {
			if (matrixFile[i][j].mtype == INIT) {
				fullLine = false;
			}
		}

		if (!fullLine) {
			// mancano dei pezzi di file, passa alla prossima riga
			continue;
		}

		// recupera path del file completo, aggiungi "_out" e aprilo
		char *temp = (char *)malloc((strlen(matrixFile[i][0].file_path) + 5) * sizeof(char)); // aggiungo lo spazio per _out
		if (temp == NULL) {
			print_msg("[server.c:main] malloc failed\n");
			exit(1);
		}
		strcpy(temp, matrixFile[i][0].file_path);
		strcat(temp, "_out"); // aggiungo _out
		int file = open(temp, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

		if (file == -1) {
			ErrExit("open failed");
		}

		// prepara l'output e scrivilo sul file
		for (int j = 0; j < 4; j++) {
			char * stampa = costruisciStringa(matrixFile[i][j]);
			if (write(file, stampa, strlen(stampa) * sizeof(char)) == -1) {
				ErrExit("write output file failed");
			}

			if (write(file, "\n\n", 2) == -1) {
				ErrExit("write newline to output file failed");
			}
			free(stampa);
		}

		close(file);
		free(temp);

		// segna la riga come letta
		for (int j = 0; j < 4; j++) {
			matrixFile[i][j].mtype = INIT;
		}

		// se cerco file completi ad ogni arrivo di una parte di file,
		// trovero' sempre al massimo un file completo: smetti di scorrere la matrice
		break;
	}
}



//---------------------------------------------------
// MAIN

int main(int argc, char * argv[]) {

	// setting of signal handler for closing IPCs
	if (signal(SIGINT, SignalHandlerTerm) == SIG_ERR) {
		ErrExit("Change of signal handler failed");
	}

	//MsgQueue
	msqid = msgget(MSQ_KEY, IPC_CREAT | S_IRUSR | S_IWUSR);

	//Setting MSQ Limit 
	struct msqid_ds ds;
	if (msgctl(msqid, IPC_STAT, &ds) == -1)
		ErrExit("msgctl STAT");
	ds.msg_qbytes = sizeof(msg_t) * IPC_MAX_MSG;
	if (msgctl(msqid, IPC_SET, &ds) == -1) {
		if (errno != EPERM) {
			ErrExit("msgctl SET");
		}
		else {
			printf("Not enough permissions to set new config\n");
		}
	}



	//SharedMemory
	shmid = alloc_shared_memory(SHM_KEY, IPC_MAX_MSG * sizeof(msg_t));
	shm_ptr = (msg_t *)attach_shared_memory(shmid, IPC_CREAT | S_IRUSR | S_IWUSR);
	if (shm_check_id == -1)
		shm_check_id = alloc_shared_memory(SHM_CHECK_KEY, IPC_MAX_MSG * sizeof(int));
	if (shm_check_id == -1)
		ErrExit("Error Allocation SHDMEM CHECKID");
	if (shm_check_ptr == NULL)
		shm_check_ptr = (int *)attach_shared_memory(shm_check_id, S_IRUSR | S_IWUSR);

	//SemaphoreSet
	semid = create_sem_set(SEM_KEY);

	//generate the 2 FIFOs (FIFO1 and FIFO2) and the needed IPCs ( 1 MsqQueue, 1 SharedMemory and a Semaphore set)
	//FIFO1
	fd_fifo1 = create_fifo(FIFO1_PATH, 'r'); //Server wants to read
	
	//FIFO2
	make_fifo(FIFO2_PATH);
	fd_fifo2 = open(FIFO2_PATH, O_RDONLY | O_NONBLOCK);
	while (true) {



		semOp(semid, FIFO1SEM, -1);
		//Server waits for number of files arrival
		msg_t n_files;
		if (read(fd_fifo1, &n_files, sizeof(msg_t)) == -1)
			ErrExit("Read failed");

		int n = 0;
		if ((n = atoi(n_files.msg_body)) == 0)
			ErrExit("Atoi failed!");
		printf("number of files: %d \n", n);

		// write the OK message on shared memory
		msg_t received_msg = { .msg_body = "OK",.mtype = N_FILES,.sender_pid = getpid() };


		// SC
		shm_ptr[0] = received_msg;
		// END SC
		semOp(semid, SHAREDMEMSEM, 1);

		// Inizializzo la matrice contenente i pezzi di file
		matrixFile=(msg_t **)malloc(n*sizeof(msg_t *));
        for(int i=0; i<n;i++)
            matrixFile[i]=(msg_t *)malloc(4*sizeof(msg_t));

        msg_t empty;
        empty.mtype = INIT;
        //inizializzo i valori del percorso per evitare di fare confronti con null
        for (int i = 0; i < BUFFER_SZ+1; i++){
			empty.file_path[i] = '\0';
		}
        //riempio la matrice con una struttura che mi dice se le celle sono vuote
        for(int i=0;i<n;i++){
			for(int j=0; j<4;j++){
				matrixFile[i][j]=empty;
			}
		}

		// Before starting ciclically waiting for messages from the 4 channels
		// I have to make Fifo non-blocking
		printf("making Non blocking fifo\n");
		semOp(semid, FIFO1SEM, -1);
		semOp(semid, FIFO2SEM, -1);
		blockFifo(fd_fifo1, 0); // 0 = false
		blockFifo(fd_fifo2, 0);
		semOp(semid, FIFO1SEM, 1);
		semOp(semid, FIFO2SEM, 1);
		printf("They are non blocking\n");



		// Ciclically waiting for messages
		int total_number_parts = n * 4; // total number of parts I have to receive
		int received_parts = 0;


		while (received_parts < total_number_parts) {

			// Save sender PID, message body of the message part, and file path
			msg_t channel1, channel2, channel3;

			//leggo da fifo1 la prima parte del file
			if (read(fd_fifo1, &channel1, sizeof(channel1)) != -1) {
				printf("[Parte1, del file %s spedita dal processo %d tramite FIFO1]\n%s\n", channel1.file_path, channel1.sender_pid, channel1.msg_body);
				semOp(semid, 7, 1);
				//addToMatrix(channel1, n);
				//findAndMakeFullFiles(n);
				received_parts++;
			}

			//leggo da fifo2 la seconda parte del file
			if (read(fd_fifo2, &channel2, sizeof(channel2)) != -1) {
				printf("[Parte2,del file %s spedita dal processo %d tramite FIFO2]\n%s\n", channel2.file_path, channel2.sender_pid, channel2.msg_body);
				semOp(semid, 8, 1);
				//addToMatrix(channel2, n);
				//findAndMakeFullFiles(n);
				received_parts++;
			}

			//leggo dalla coda di messaggi la terza parte del file
			if (msgrcv(msqid, &channel3, sizeof(struct msg_t) - sizeof(long), MSQ_PART, IPC_NOWAIT) != -1) {
				printf("[Parte3,del file %s spedita dal processo %d tramite MsgQueue]\n%s\n", channel3.file_path, channel3.sender_pid, channel3.msg_body);
				semOp(semid, 9, 1);
				//addToMatrix(channel3, n);
				//findAndMakeFullFiles(n);
				received_parts++;
			}


		/*	semOp(semid, SHAREDMEMSEM, -1);
			printf("[Parte 4, del file %s , spedita dal processo %d tramite ShdMem]%s \n", shm_ptr[0].file_path, shm_ptr[0].sender_pid, shm_ptr[0].msg_body);
			received_parts++;*/
			//exit(0);
			semOp(semid, 6, -1);
			for (int i = 0; i < IPC_MAX_MSG; i++) {
				if (shm_check_ptr[i] == 1) {
					shm_check_ptr[i] = 0;
					printf("[Parte 4, del file %s , spedita dal processo %d tramite ShdMem]%s \n", shm_ptr[i].file_path, shm_ptr[i].sender_pid, shm_ptr[i].msg_body);
					received_parts++;
				}
			}
			semOp(semid, 6, 1);
		}
		//HO RICEVUTO TUTTE LE PARTI, LE DEVO RIUNIRE




		// sending message to client: I've ended my work
		msg_t end_msg = { .msg_body = "FINISHED",.mtype = FINISHED,.sender_pid = getpid() };
		msgsnd(msqid, &end_msg, sizeof(struct msg_t) - sizeof(long), 0);

		// RICORDATI DI RENDERE DI NUOVO BLOCCANTI LE FIFO SE NO POI NON FUNZIA PIU'

		// Wait again for a new number of files to receive
	}


	return 0;
}

