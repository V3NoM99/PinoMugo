/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/msg.h>

#include "defines.h"
#include "err_exit.h"



char * int_to_string(int value){
    int needed = snprintf(NULL, 0, "%d", value);

    char * string = (char *) malloc(needed+1);
    if (string == NULL){
        ErrExit("<CLIENT> malloc failed\n");
    }

    snprintf(string, needed+1, "%d", value);
    return string;
}