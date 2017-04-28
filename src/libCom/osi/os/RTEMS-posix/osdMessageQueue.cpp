/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* Copyright (c) 2017 Fritz-Haber-Institut der Max-Planck-Gesellschaft
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 *      Author  W. Eric Norum
 *              Heinz Junkes
 *
 * Adapted to rtems4.12
 *   -> posix message queues 
 *   remove all internal calls (_xxx), remove e.g. Objects_Locations etc.
 */

/*
 * We want to access information which is
 * normally hidden from application programs.
 */
#define __RTEMS_VIOLATE_KERNEL_VISIBILITY__ 1

#define epicsExportSharedSymbols
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rtems.h>
#include <rtems/error.h>
#include "epicsMessageQueue.h"
#include "errlog.h"
#include <epicsAtomic.h>

#include <errno.h>
#include <mqueue.h>
#include <fcntl.h>

epicsShareFunc epicsMessageQueueId epicsShareAPI
epicsMessageQueueCreate(unsigned int capacity, unsigned int maximumMessageSize)
{
  struct mq_attr the_attr;
  epicsMessageQueueId id = (epicsMessageQueueId)calloc(1, sizeof(*id));

  epicsAtomicIncrIntT(&id->idCnt);
  sprintf(id->name, "MQ_%010d",epicsAtomicGetIntT(&id->idCnt));    
  the_attr.mq_maxmsg = capacity;
  the_attr.mq_msgsize = maximumMessageSize;
  id->id = mq_open(id->name, O_RDWR | O_CREAT | O_EXCL, 0644, &the_attr);
  if (id->id < 0) { 
    errlogPrintf ("Can't create message queue: %s\n", strerror (errno));
    return NULL;
  }
  return id;
}

epicsShareFunc void epicsShareAPI epicsMessageQueueDestroy(
    epicsMessageQueueId id)
{
  int rv;
  rv = mq_close(id->id);
  if( rv ) { 
    errlogPrintf("epicsMessageQueueDestroy mq_close failed: %s\n",
                         strerror(rv));
  }
  rv = mq_unlink(id->name);
  if( rv ) { 
    errlogPrintf("epicsMessageQueueDestroy mq_unlink %s failed: %s\n",
                         id->name, strerror(rv));
  }
  free(id);
}

epicsShareFunc int epicsShareAPI epicsMessageQueueTrySend(
    epicsMessageQueueId id,
    void *message,
    unsigned int messageSize)
{
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return mq_timedsend(id->id, (char const *)message, messageSize, 0, &ts); 
}

epicsShareFunc int epicsShareAPI epicsMessageQueueSendWithTimeout(
    epicsMessageQueueId id,
    void *message,
    unsigned int messageSize,
    double timeout)
{
  struct timespec ts;
  unsigned long micros;
    
  // assume timeout in sec
  micros = (unsigned long)(timeout * 1000000.0);
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += micros / 1000000L;
  ts.tv_nsec += (micros % 1000000L) * 1000L;

  return  mq_timedsend (id->id, (const char *)message, messageSize, 0, &ts);
}

epicsShareFunc int epicsShareAPI epicsMessageQueueTryReceive(
    epicsMessageQueueId id,
    void *message,
    unsigned int size)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return mq_timedreceive(id->id, (char *)message, size, NULL, &ts);
}

epicsShareFunc int epicsShareAPI epicsMessageQueueReceiveWithTimeout(
    epicsMessageQueueId id,
    void *message,
    unsigned int size,
    double timeout)
{
    unsigned long micros;
    struct timespec ts;
    
    micros = (unsigned long)(timeout * 1000000.0);
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += micros / 1000000L;
    ts.tv_nsec += (micros % 1000000L) * 1000L;

    return mq_timedreceive(id->id, (char *)message, size, NULL, &ts);
}

epicsShareFunc int epicsShareAPI epicsMessageQueuePending(
            epicsMessageQueueId id)
{
  int rv;
  struct mq_attr the_attr;

  rv = mq_getattr(id->id, &the_attr);    
  if (rv) {
    errlogPrintf("Epics Message queue %x (%s) get attr failed: %s\n",
                         (unsigned int)id->id, id->name, strerror(rv));
        return -1;
    }
    return the_attr.mq_curmsgs;
}

epicsShareFunc void epicsShareAPI epicsMessageQueueShow(
            epicsMessageQueueId id,
                int level)
{
  int rv;
  struct mq_attr the_attr;

  rv = mq_getattr(id->id, &the_attr);    
  if (rv) {
    errlogPrintf("Epics Message queue %x (%s) get attr failed: %s\n",
                         (unsigned int)id->id, id->id, strerror(rv));
    }

    printf("Message Queue Used:%ld  Max Msg:%lu", the_attr.mq_curmsgs, the_attr.mq_maxmsg);
    if (level >= 1)
       printf("  Maximum size:%lu", the_attr.mq_msgsize);

    printf("\n");
}
