/// @file fifo.h
/// @brief Contiene la definizioni di variabili e
///         funzioni specifiche per la gestione delle FIFO.

#pragma once

/**
 * @brief Crea o ottiene una IPC FIFO usando mkfifo.
 *
 * @param path Percorso file usato dalla FIFO.
*/
void make_fifo(char * path);


/**
 * @brief Facilita la creazione di una FIFO (o ne ottiene una esistente) in modalita' lettura o scrittura.
 *
 * @param path Percorso file usato dalla FIFO.
 * @param mode 'r' (read) oppure 'w' (write)
 * @return int Descrittore della FIFO
 */
int create_fifo(char * path, char mode);

/**
 * @brief Rende bloccante o non bloccante la FIFO
 *
 * @param fd File Descriptor della fifo.
 * @param blockingFlag 1 blocca 0 sblocca
 * @return int Descrittore della FIFO
 */
int blockFifo(int fd, int blockingFlag);