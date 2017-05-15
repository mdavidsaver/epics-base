/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* Copyright (c) 2017 Fritz-Haber-Institut der Max-Planck-Gesellschaft
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <pthread.h>

struct epicsEventOSD {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int             isFull;
};
