/*************************************************************************\
* SPDX-License-Identifier: EPICS
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <epicsThread.h>
#include "epicsUnitTest.h"
#include "testMain.h"
#include <stdio.h>
#include <string.h>

#ifdef __rtems__

/* RTEMS is posix but currently does not use the pthread API */
MAIN(nonEpicsThreadPriorityTest)
{
    testPlan(1);
    testSkip(1, "Platform does not use pthread API");
    return testDone();
}

#else

static void *nonEpicsTestFunc(void *arg)
{
    unsigned int pri;
    // epicsThreadGetIdSelf()  creates an EPICS context
    // verify that the priority computed by epics context
    // is OK
    pri = epicsThreadGetPriority( epicsThreadGetIdSelf() );
    if ( ! testOk( 0 == pri, "'createImplicit' assigned correct priority (%d) to non-EPICS thread", pri) ) {
        return 0;
    }

    return (void*)1;
}


static void testFunc(void *arg)
{
    epicsEventId        ev = (epicsEventId)arg;
    int                 policy;
    struct sched_param  param;
    int                 status;
    pthread_t           tid;
    void               *rval;
    pthread_attr_t      attr;
    unsigned char       expect_success;

    status = pthread_getschedparam(pthread_self(), &policy,&param);
    if ( status ) {
        testAbort("pthread_getschedparam failed");
    }

    expect_success = SCHED_FIFO == policy;

    if ( pthread_attr_init( &attr ) ) {
        testAbort("pthread_attr_init failed");
    }
    if ( pthread_attr_setinheritsched( &attr, PTHREAD_EXPLICIT_SCHED ) ) {
        testAbort("pthread_attr_setinheritsched failed");
    }
    if ( pthread_attr_setschedpolicy ( &attr, SCHED_OTHER            ) ) {
        testAbort("pthread_attr_setschedpolicy failed");
    }
    param.sched_priority = 0;
    if ( pthread_attr_setschedparam  ( &attr, &param                 ) ) {
        testAbort("pthread_attr_setschedparam failed");
    }

    if(!expect_success)
        testTodoBegin("nonEpicsThreadPriorityTest must be executed with privileges to use SCHED_FIFO");

    if ( pthread_create( &tid, &attr, nonEpicsTestFunc, 0 ) ) {
        testAbort("pthread_create failed");
    }

    if ( pthread_join( tid, &rval ) ) {
        testAbort("pthread_join failed");
    }

    testTodoEnd();

    epicsEventSignal( ev );
}

MAIN(nonEpicsThreadPriorityTest)
{
    testPlan(2);
    epicsEventId testComplete = epicsEventMustCreate(epicsEventEmpty);
    epicsThreadMustCreate("nonEpicsThreadPriorityTest", epicsThreadPriorityLow,
        epicsThreadGetStackSize(epicsThreadStackMedium),
        testFunc, testComplete);
    epicsEventWaitStatus status = epicsEventWait(testComplete);
    testOk(status == epicsEventWaitOK,
        "epicsEventWait returned %d", status);
    return testDone();
}
#endif
