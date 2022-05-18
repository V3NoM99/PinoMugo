/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include <sys/stat.h>
#include <sys/sem.h>
#include <errno.h>

#include "semaphore.h"
#include "err_exit.h"


//--------------GESTIONE SEMAFORI------------------
int createSemaphores(key_t key, int n_sem) {
    int semid = semget(key, n_sem, IPC_CREAT | S_IRUSR | S_IWUSR);

    if (semid == -1)
        ErrExit("semget failed");
    return semid;
}

int getSemaphores(key_t key, int n_sem) {
    int semid = semget(key, n_sem, S_IRUSR | S_IWUSR);

    if (semid == -1)
        ErrExit("semget failed");
    return semid;
}

void semDelete(int semid) {
    if (semctl(semid, 0/*ignored*/, IPC_RMID, 0/*ignored*/) == -1) {
        ErrExit("semctl failed");
    }
}

//---------------MODIFICA SEMAFORI---------------------

void semOp (int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};

    if (semop(semid, &sop, 1) == -1)
        ErrExit("semop failed");
}


int semOpNoWait(int semid, unsigned short sem_num, short sem_op) {
    struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = IPC_NOWAIT};
    if (semop(semid, &sop, 1) == -1){
        if (errno == EAGAIN){
            return -1;
        }
        else{
            ErrExit("semop failed");
        }
    }

    return 0;
}

void semSetVal(int semid, int sem_num, int val) {
    union semun arg;
    arg.val = val;

    if (semctl(semid, sem_num, SETVAL, arg) == -1) {
        ErrExit("semctl SETVAL");
    }
}


void semSetAll(int semid, short unsigned int values[]) {
    union semun arg;
    arg.array = values;

    // Inizializza il set di semafori
    if (semctl(semid, 0/*ignored*/, SETALL, arg) == -1)
        ErrExit("semctl SETALL");
}

void semSetPerm(int semid, struct semid_ds arg) {
    if (semctl(semid, 0 /*ignored*/, IPC_SET, arg) == -1)
        ErrExit("semctl IPC_SET failed");
}