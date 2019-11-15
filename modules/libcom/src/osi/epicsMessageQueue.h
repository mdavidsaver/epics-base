/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/*
 *      Author  W. Eric Norum
 *              norume@aps.anl.gov
 *              630 252 4793
 */

/*
 * Interthread message passing
 */
#ifndef epicsMessageQueueh
#define epicsMessageQueueh

#include "epicsAssert.h"
#include "libComAPI.h"

typedef struct epicsMessageQueueOSD *epicsMessageQueueId;

#ifdef __cplusplus

class LIBCOM_API epicsMessageQueue {
public:
    epicsMessageQueue ( unsigned int capacity,
                        unsigned int maximumMessageSize );
    ~epicsMessageQueue ();
    int trySend ( void *message, unsigned int messageSize );
    int send ( void *message, unsigned int messageSize);
    int send ( void *message, unsigned int messageSize, double timeout );
    int tryReceive ( void *message, unsigned int size );
    int receive ( void *message, unsigned int size );
    int receive ( void *message, unsigned int size, double timeout );
    void show ( unsigned int level = 0 );
    unsigned int pending ();

private: /* Prevent compiler-generated member functions */
    /* default constructor, copy constructor, assignment operator */
    epicsMessageQueue();
    epicsMessageQueue(const epicsMessageQueue &);
    epicsMessageQueue& operator=(const epicsMessageQueue &);

    epicsMessageQueueId id;
};

extern "C" {
#endif /*__cplusplus */

LIBCOM_API epicsMessageQueueId LIBCOMSTD_API epicsMessageQueueCreate(
    unsigned int capacity,
    unsigned int maximumMessageSize);
LIBCOM_API void LIBCOMSTD_API epicsMessageQueueDestroy(
    epicsMessageQueueId id);
LIBCOM_API int LIBCOMSTD_API epicsMessageQueueTrySend(
    epicsMessageQueueId id,
    void *message,
    unsigned int messageSize);
LIBCOM_API int LIBCOMSTD_API epicsMessageQueueSend(
    epicsMessageQueueId id,
    void *message,
    unsigned int messageSize);
LIBCOM_API int LIBCOMSTD_API epicsMessageQueueSendWithTimeout(
    epicsMessageQueueId id,
    void *message,
    unsigned int messageSize,
    double timeout);
LIBCOM_API int LIBCOMSTD_API epicsMessageQueueTryReceive(
    epicsMessageQueueId id,
    void *message,
    unsigned int size);
LIBCOM_API int LIBCOMSTD_API epicsMessageQueueReceive(
    epicsMessageQueueId id,
    void *message,
    unsigned int size);
LIBCOM_API int LIBCOMSTD_API epicsMessageQueueReceiveWithTimeout(
    epicsMessageQueueId id,
    void *message,
    unsigned int size,
    double timeout);
LIBCOM_API int LIBCOMSTD_API epicsMessageQueuePending(
    epicsMessageQueueId id);
LIBCOM_API void LIBCOMSTD_API epicsMessageQueueShow(
    epicsMessageQueueId id,
    int level);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#include "osdMessageQueue.h"

#endif /* epicsMessageQueueh */
