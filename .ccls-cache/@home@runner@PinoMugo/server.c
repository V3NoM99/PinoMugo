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

//Shared memory pointer
msg_t * shm_ptr = NULL;

// SERVER INTERNAL FUNCTIONS

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


    // close message queue
    if (msqid != -1) {
        if (msgctl(msqid, IPC_RMID, NULL) == -1){
            ErrExit("SERVER: msgctl failed");
        }
    }

    

	exit(0);
}

int main(int argc, char * argv[]) {

  	// setting of signal handler for closing IPCs
  	if (signal(SIGINT, SignalHandlerTerm) == SIG_ERR) {
    	ErrExit("Change of signal handler failed");
  	}

  	//MsgQueue
  	msqid = msgget(MSQ_KEY, IPC_CREAT | S_IRUSR | S_IWUSR);
  	//TODO : c'è da limitare numero messagi coda a 50 look how to do it

  	//SharedMemory
  	shmid = alloc_shared_memory(SHM_KEY, IPC_MAX_MSG * sizeof(msg_t));
  	shm_ptr = (msg_t *) attach_shared_memory(shmid, IPC_CREAT | S_IRUSR | S_IWUSR);

  	//SemaphoreSet
  	semid = createSemaphores(SEM_KEY, 2);
  	semSetVal(semid,0,0);
  	semSetVal(semid,1,0);
    
  	while(true){
          
  		//generate the 2 FIFOs (FIFO1 and FIFO2) and the needed IPCs ( 1 MsqQueue, 1 SharedMemory and a Semaphore set)
  		//FIFO1
        fd_fifo1 = create_fifo(FIFO1_PATH, 'r');   //Server wants to read

        semOp(semid, 0, -1);
        //Server waits for number of files arrival
        msg_t n_files;
        if (read(fd_fifo1, &n_files, sizeof(msg_t)) == -1)
            ErrExit("Read failed");
        
        int n = 0;
        if ((n = atoi(n_files.msg_body))==0)
            ErrExit("Atoi failed!");
        printf("number of files: %d \n", n);

      	// write the OK message on shared memory
      	msg_t received_msg = {.msg_body = "OK", .mtype = N_FILES, .sender_pid = getpid()};

        
      	// SC
      	shm_ptr[0] = received_msg;
      	// END SC
      	semOp(semid, 1, 1);
  	}


  	return 0;
}
