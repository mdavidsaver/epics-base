/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/* Author:  Marty Kraimer Date:    18JAN2000 */

/* This is part of the posix implementation of epicsThread */

#define epicsExportSharedSymbols
#include "epicsStdio.h"
#include "ellLib.h"
#include "epicsEvent.h"
#include "epicsThread.h"

epicsShareDef EPICS_THREAD_HOOK_ROUTINE epicsThreadHookDefault;
epicsShareDef EPICS_THREAD_HOOK_ROUTINE epicsThreadHookMain;

void epicsThreadShowInfo(epicsThreadOSD *pthreadInfo, unsigned int level)
{
    if(!pthreadInfo) {
        fprintf(epicsGetStdout(),"            NAME       EPICS ID   "
            "PTHREAD ID   OSIPRI  OSSPRI  STATE  STACKSIZE\n");
    } else {
        struct sched_param param;
        int policy;
        int priority = 0;
        size_t stackSize = 0;

        if(pthreadInfo->tid) {
            int status;
            status = pthread_getschedparam(pthreadInfo->tid,&policy,&param);
            if(!status) priority = param.sched_priority;
            status = pthread_attr_getstacksize( &pthreadInfo->attr,&stackSize);
            if(status) stackSize = 0;
        }
        fprintf(epicsGetStdout(),"%16.16s %14p   0x%08X    %3d%8d %8.8s  %9d\n",
             pthreadInfo->name,(void *)
             pthreadInfo,(unsigned long)pthreadInfo->tid,
             pthreadInfo->osiPriority,priority,
             pthreadInfo->isSuspended?"SUSPEND":"OK",
             stackSize);
    }
}

