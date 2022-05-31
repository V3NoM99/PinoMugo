/// @file fifo.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle FIFO.

#include "err_exit.h"
#include "fifo.h"

#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "err_exit.h"
#include "fifo.h"
#include "defines.h"


void make_fifo(char * path) {
    if (mkfifo(path, S_IRUSR | S_IWUSR) == -1) {
        if (errno != EEXIST) {
            ErrExit("[fifo.c:make_fifo] mkfifo failed");
        }
    }
}


int create_fifo(char * path, char mode) {
    int fifo1_fd = -1;

    make_fifo(path);
    printf("FIFO created/obtained\n");

    if (mode == 'r') {
        fifo1_fd = open(path, O_RDONLY );
        if (fifo1_fd == -1) {
            ErrExit("[fifo.c:create_fifo] open FIFO1 failed (read mode)");
        }
    }
    else if (mode == 'w') {
        fifo1_fd = open(path, O_WRONLY);
        if (fifo1_fd == -1) {
            ErrExit("[fifo.c:create_fifo] open FIFO1 failed (write mode)");
        }
    }
    else {
        printf("[fifo.c:create_fifo] mode should be 'r' or 'w'\n"); //USIAMO LA PRINTF O PRINT_MSG CON LA WRITE???
        exit(1);
    }

    return fifo1_fd;
}

int blockFifo(int fd, int blockingFlag) {
    // Save the current flags
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return 0;

    if (blockingFlag)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;

    return fcntl(fd, F_SETFL, flags) != -1;
}
