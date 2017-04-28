/*************************************************************************\
 * * Copyright (c) 2002 The University of Chicago, as Operator of Argonne
 * *     National Laboratory.
 * * Copyright (c) 2002 The Regents of the University of California, as
 * *     Operator of Los Alamos National Laboratory.
 * * Copyright (c) 2017 Fritz-Haber-Institut der Max-Planck-Gesellschaft
 * * EPICS BASE is distributed subject to a Software License Agreement found
 * * in file LICENSE that is included with this distribution.
 * \*************************************************************************/
/*
 *      Author  W. Eric Norum
 *              Heinz Junkes
 *
 * Eric's note : Very thin shims around vxWorks routines
 *
 * Adapted to rtems4.12
 *   -> posix message queues 
 */

#include <rtems.h>
#include <mqueue.h>

struct epicsMessageQueueOSD {
    mqd_t id;
    char  name[24];
    int	  idCnt;

};

#define epicsMessageQueueSend(q,m,l) (mq_send((q)->id, (const char*)(m), (l), 0))
#define epicsMessageQueueReceive(q,m,s) (mq_receive((q)->id, (char*)(m), (s), NULL))
