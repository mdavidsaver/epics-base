/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* SPDX-License-Identifier: EPICS
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef osdThreadh
#define osdThreadh

#include <pthread.h>
#include <sys/types.h>

#include "libComAPI.h"
#include "ellLib.h"
#include "epicsEvent.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct epicsThreadOSD {
    // guarded by listLock
    ELLNODE            node;
    // atomic
    int                refcnt;
    // const after creation
    pthread_t          tid;
#ifdef __linux__
    pid_t              lwpId;
#endif
    // TODO: unsafe
    pthread_attr_t     attr;
    struct sched_param schedParam;
    int                schedPolicy;
    // const after creation
    EPICSTHREADFUNC    createFunc;
    void              *createArg;
    epicsEventId       suspendEvent;
    // TODO: unsafe
    int                isSuspended;
    // const after creation
    int                isEpicsThread;
    int                isRealTimeScheduled;
    // guard by listLock
    int                isOnThreadList;
    // atomic
    int                isRunning;
    // thread local
    int                isOkToBlock;
    // TODO: unsafe
    unsigned int       osiPriority;
    // atomic
    int                joinable;
    // const after creation
    char               name[1];     /* actually larger */
} epicsThreadOSD;

LIBCOM_API pthread_t epicsThreadGetPosixThreadId(epicsThreadId id);
LIBCOM_API int epicsThreadGetPosixPriority(epicsThreadId id);

#ifdef __cplusplus
}
#endif

#endif /* osdThreadh */
