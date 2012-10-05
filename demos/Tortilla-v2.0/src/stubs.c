/*
 * stubs.c
 *
 *  Created on: Mar 7, 2012
 *      Author: Xo
 */

#include "ch.h"

#include <stdio.h>
#include <stdint.h>

void _exit(int status) {
    (void) status;
    chSysHalt();

    while (1)
        ;
}

pid_t _getpid(void) {
    return 1;
}

void _kill(pid_t id) {
    (void) id;
}

void *_sbrk(intptr_t increment) {
    (void) increment;
    return (void *)-1;
}
