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

#include <errno.h>
#include <mqueue.h>
#include <fcntl.h>

epicsShareFunc epicsMessageQueueId epicsShareAPI
epicsMessageQueueCreate(unsigned int capacity, unsigned int maximumMessageSize)
{

  rtems_interrupt_level level;

  epicsMessageQueueId mqDes;
  struct mq_attr the_attr;

  static char c1 = 'a';
  static char c2 = 'a';
  static char c3 = 'a';
  static char name[6];

  sprintf(name, "MQ_%c%c%c",c3,c2,c1);    
  the_attr.mq_maxmsg = capacity;
  the_attr.mq_msgsize = maximumMessageSize;
  mqDes = (epicsMessageQueueId)mq_open(name, O_RDWR | O_CREAT, 0644, &the_attr);
  if (mqDes < 0) { 
    errlogPrintf ("Can't create message queue: %s\n", strerror (errno));
    return NULL;
  }

  rtems_interrupt_disable (level);
  if (c1 == 'z') {
      if (c2 == 'z') {
          if (c3 == 'z') {
              c3 = 'a';
          }
          else {
              c3++;
          }
          c2 = 'a';
      }
      else {
          c2++;
      }
      c1 = 'a';
  }
  else {
      c1++;
  }
  rtems_interrupt_enable (level);

  return mqDes;
}

epicsShareFunc int epicsShareAPI epicsMessageQueueTrySend(
    epicsMessageQueueId id,
    void *message,
    unsigned int messageSize)
{
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  return mq_timedsend((mqd_t)id, (char const *)message, messageSize, 0, &ts); 
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

  return  mq_timedsend ((mqd_t)id, (const char *)message, messageSize, 0, &ts);
}

epicsShareFunc int epicsShareAPI epicsMessageQueueTryReceive(
    epicsMessageQueueId id,
    void *message,
    unsigned int size)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return mq_timedreceive((mqd_t)id, (char *)message, size, NULL, &ts);
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

    return mq_timedreceive((mqd_t)id, (char *)message, size, NULL, &ts);
}

epicsShareFunc int epicsShareAPI epicsMessageQueuePending(
            epicsMessageQueueId id)
{
  int rv;
  struct mq_attr the_attr;

  rv = mq_getattr((mqd_t)id, &the_attr);    
  if (rv) {
    errlogPrintf("Epics Message queue %x get attr failed: %s\n",
                         (unsigned int)id, strerror(rv));
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

  rv = mq_getattr((mqd_t)id, &the_attr);    
  if (rv) {
    errlogPrintf("Epics Message queue %x get attr failed: %s\n",
                         (unsigned int)id, strerror(rv));
    }

    printf("Message Queue Used:%ld  Max Msg:%lu", the_attr.mq_curmsgs, the_attr.mq_maxmsg);
    if (level >= 1)
       printf("  Maximum size:%lu", the_attr.mq_msgsize);

    printf("\n");
}
