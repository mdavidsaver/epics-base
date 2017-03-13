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

#define epicsMessageQueueDestroy(q) (mq_close((mqd_t)(q)))

#define epicsMessageQueueSend(q,m,l) (mq_send((mqd_t)(q), (const char*)(m), (l), 0))
#define epicsMessageQueueReceive(q,m,s) (mq_receive((mqd_t)(q), (char*)(m), (s), NULL))

/*
#define epicsMessageQueueTryReceive(q,m,s) (msgQReceive((MSG_Q_ID)(q), (char*)(m), (s), NO_WAIT))

#define epicsMessageQueuePending(q) (msgQNumMsgs((MSG_Q_ID)(q)))
#define epicsMessageQueueShow(q,l) (msgQShow((MSG_Q_ID)(q),(l)))
*/
