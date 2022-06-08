/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.

#pragma once

#include <stdbool.h>
#include <sys/types.h>
#include <sys/ipc.h>

//-------SIZES------

#define BUFFER_SZ 255

// maximum number of messages per IPC
#define IPC_MAX_MSG 50

// Max dim messages
#define MSG_SZ 4096

/// Messages parts
#define MSG_PARTS 4

/// Max message part size
#define MSG_PART_SZ (MSG_SZ / MSG_PARTS)



//VARIABLES' DEFINITION

//FIFOs Paths
#define FIFO1_PATH "/tmp/FIFO1"
#define FIFO2_PATH "/tmp/FIFO2"

//KEYS

#define MSQ_KEY (key_t)111111

#define SHM_KEY (key_t)222222

#define SEM_KEY (key_t)333333

#define SEM_KEY_START (key_t)444444

#define SHM_CHECK_KEY (key_t)555555


//SEMAFORI
#define FIFO1SEM		0
#define FIFO2SEM		1
#define MUTEXFILE		2
#define SHMOK		   	3
#define BLOCKFINISHED	4
#define SEMNUMFILES		5
#define SHMSEM			6
#define FIFO1LIMIT		7
#define FIFO2LIMIT		8
#define MSQLIMIT		9


// MTYPES
#define FIFO1_PART 1
#define FIFO2_PART 2
#define MSQ_PART 3
#define SHM_PART 4
#define N_FILES 5
#define FINISHED 6

#define INIT -1

//---------Messages structure---------
typedef struct msg_t {
    // this mtype is used by MessageQueue
    long mtype;
    // sender's PID
    pid_t sender_pid;
    // File path
    char file_path[BUFFER_SZ+2];
    // Message body
    char msg_body[MSG_PART_SZ+2];
} msg_t;



void print_msg(char * msg);

char * int_to_string(int value);
