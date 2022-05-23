/// @file semaphore.h
/// @brief Contiene la definizioni di variabili e funzioni
///         specifiche per la gestione dei SEMAFORI.

#pragma once

#ifndef _SEMAPHORE_HH
#define _SEMAPHORE_HH

// definition of the union semun
union semun {
    int val;
    struct semid_ds * buf;
    unsigned short * array;
};

/**
 * @brief Crea un set di semafori
 *
 * @param key Chiave IPC
 * @param n_sem Numero semafori da creare
*/
int createSemaphores(key_t key, int n_sem);

/**
 * @brief Restituisce un insieme di semafori gia' creato
 *
 * @param key Chiave IPC
 * @param n_sem Numero semaforo da ottenere
*/
int getSemaphores(key_t key, int n_sem);

/**
 * @brief Funzione per modificare i valori di un set di semafori.
 *
 * @param semid Identificatore del set di semafori
 * @param sem_num Indice di un semaforo nel set
 * @param sem_op Operazione eseguita sul semaforo sem_num
*/
void semOp (int semid, unsigned short sem_num, short sem_op);


/**
 * Funzione per modificare i valori di un set di semafori in modo non bloccante.
 *
 * @param semid Identificatore del set di semafori
 * @param sem_num Indice di un semaforo nel set
 * @param sem_op Operazione eseguita sul semaforo sem_num
 * @return -1 se il semaforo ha tentato di bloccare il processo, 0 altrimenti
*/
int semOpNoWait(int semid, unsigned short sem_num, short sem_op);

/**
 * @brief Inizializza il valore del semaforo sem_num al valore val.
 *
 * @param semid Identificatore del set di semafori
 * @param sem_num Indice di un semaforo nel set
 * @param val Valore a cui impostare il semaforo sem_num
*/
void semSetVal(int semid, int sem_num, int val);


/**
 * @brief Inizializza i valori del set di semafori semid ai valori dell'array values.
 *
 * @param semid Identificatore del set di semafori
 * @param values Array di valori a cui impostare ogni semaforo del set
*/
void semSetAll(int semid, short unsigned int values[]);


/**
 * @brief Cancella il set di semafori svegliando eventuali processi in attesa.
 *
 * @param semid Identificatore del set di semafori
 */
void semDelete(int semid);


/**
 * @brief Imposta i permessi su un set di semafori.
 *
 * @param semid Identificatore del set di semafori
 * @param arg Contiene i permessi da impostare
*/
void semSetPerm(int semid, struct semid_ds arg);

#endif
