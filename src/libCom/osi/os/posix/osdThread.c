/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* Copyright (c) 2013 ITER Organization.
* Copyright (c) 2017 Fritz-Haber-Institut der Max-Planck-Gesellschaft
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

/* Author:  Marty Kraimer Date:    18JAN2000 
   	    Heinz Junkes  Date:    06APR2017

   once() and all called functions by once must use
   checkStatusOnce and checkStatusQuitOnce only
   including epicsEventCreate called by create_threadInfo 
   add epicsEventCreateOnce which can be called here 
*/

/* This is a posix implementation of epicsThread */
#include <stddef.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>
#include <sched.h>
#include <unistd.h>

#if defined(_POSIX_MEMLOCK) && _POSIX_MEMLOCK > 0
#include <sys/mman.h>
#endif

#define epicsExportSharedSymbols
#include "epicsStdio.h"
#include "ellLib.h"
#include "epicsEvent.h"
#include "epicsMutex.h"
#include "epicsString.h"
#include "epicsThread.h"
#include "cantProceed.h"
#include "errlog.h"
#include "epicsAssert.h"
#include "epicsExit.h"
#if defined(__rtems__)
#include <rtems/bspIo.h>
#include <rtems.h>
#endif

epicsShareFunc void epicsThreadShowInfo(epicsThreadOSD *pthreadInfo, unsigned int level);
epicsShareFunc void osdThreadHooksRun(epicsThreadId id);
epicsShareFunc void osdThreadHooksRunMain(epicsThreadId id);

static int mutexLock(pthread_mutex_t *id)
{
    int status;

    while(1) {
        status = pthread_mutex_lock(id);
        if(status!=EINTR) return status;
        fprintf(stderr,"pthread_mutex_lock returned EINTR. Violates SUSv3\n");
    }
}

#if defined DONT_USE_POSIX_THREAD_PRIORITY_SCHEDULING
#undef _POSIX_THREAD_PRIORITY_SCHEDULING
#endif

typedef struct commonAttr{
    pthread_attr_t     attr;
    struct sched_param schedParam;
    int                maxPriority;
    int                minPriority;
    int                schedPolicy;
    int                usePolicy;
} commonAttr;

#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && _POSIX_THREAD_PRIORITY_SCHEDULING > 0
typedef struct {
    int min_pri, max_pri;
    int policy;
    int ok;
} priAvailable;
#endif

__thread epicsThreadOSD *tls_pthreadInfo;

static pthread_mutex_t onceLock;
static pthread_mutex_t listLock;
static ELLLIST pthreadList = ELLLIST_INIT;
static commonAttr *pcommonAttr = 0;
static int epicsThreadInitOnceCalled = 0;

static void createImplicit(void);

#define checkStatus(status,message) \
if((status))  {\
    errlogPrintf("%s error %s\n",(message),strerror((status))); \
}

#define checkStatusQuit(status,message,method) \
if((status)) { \
    errlogPrintf("%s  error %s\n",(message),strerror((status))); \
    cantProceed((method)); \
}

/* The following are for use by once, which is only invoked from epicsThreadInit*/
/* Until epicsThreadInit completes errlogInit will not work                     */
/* It must also be used by init_threadInfo otherwise errlogInit could get  */
/* called recursively                                                      */
#if defined (__rtems__)
#define checkStatusOnce(status,message) \
if((status))  {\
    printk("%s error %s\n",(message),strerror((status))); }
#else
#define checkStatusOnce(status,message) \
if((status))  {\
    fprintf(stderr,"%s error %s\n",(message),strerror((status))); }
#endif

#if defined (__rtems__)
#define checkStatusOnceQuit(status,message,method) \
if((status)) { \
    printk("%s  error %s",(message),strerror((status))); \
    printk(" %s\n",method); \
    printk("epicsThreadInit cant proceed. Program exiting\n"); \
    exit(-1);\
}
#else
#define checkStatusOnceQuit(status,message,method) \
if((status)) { \
    fprintf(stderr,"%s  error %s",(message),strerror((status))); \
    fprintf(stderr," %s\n",method); \
    fprintf(stderr,"epicsThreadInit cant proceed. Program exiting\n"); \
    exit(-1);\
}
#endif


epicsShareFunc int epicsThreadGetPosixPriority(epicsThreadId pthreadInfo)
{
#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && _POSIX_THREAD_PRIORITY_SCHEDULING > 0
    double maxPriority,minPriority,slope,oss;

    if(pcommonAttr->maxPriority==pcommonAttr->minPriority)
        return(pcommonAttr->maxPriority);
    maxPriority = (double)pcommonAttr->maxPriority;
    minPriority = (double)pcommonAttr->minPriority;
    slope = (maxPriority - minPriority)/100.0;
    oss = (double)pthreadInfo->osiPriority * slope + minPriority;
    return((int)oss);
#else
    return 0;
#endif /* _POSIX_THREAD_PRIORITY_SCHEDULING */
}

static void setSchedulingPolicy(epicsThreadOSD *pthreadInfo,int policy)
{
#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && _POSIX_THREAD_PRIORITY_SCHEDULING > 0
    int status;

    if(!pcommonAttr->usePolicy) return;

    status = pthread_attr_getschedparam(
        &pthreadInfo->attr,&pthreadInfo->schedParam);
    checkStatusOnce(status,"pthread_attr_getschedparam");
    pthreadInfo->schedParam.sched_priority = epicsThreadGetPosixPriority(pthreadInfo);
    pthreadInfo->schedPolicy = policy;
    status = pthread_attr_setschedpolicy(
        &pthreadInfo->attr,policy);
    checkStatusOnce(status,"pthread_attr_setschedpolicy");
    status = pthread_attr_setschedparam(
        &pthreadInfo->attr,&pthreadInfo->schedParam);
    checkStatusOnce(status,"pthread_attr_setschedparam");
    status = pthread_attr_setinheritsched(
        &pthreadInfo->attr,PTHREAD_EXPLICIT_SCHED);
    checkStatusOnce(status,"pthread_attr_setinheritsched");
#endif /* _POSIX_THREAD_PRIORITY_SCHEDULING */
}


epicsShareFunc epicsEventId epicsEventCreateOnce(epicsEventInitialState init)
{
    epicsEventId pevent = malloc(sizeof(*pevent));
    if (pevent) {
        int status = pthread_mutex_init(&pevent->mutex, 0);

        pevent->isFull = (init == epicsEventFull);
        if (status) {
            checkStatus(status, "pthread_mutex_init");
        } else {
            status = pthread_cond_init(&pevent->cond, 0);
            if (!status)
                return pevent;
            checkStatus(status, "pthread_cond_init");
            status = pthread_mutex_destroy(&pevent->mutex);
            checkStatus(status, "pthread_mutex_destroy");
        }
        free(pevent);
    }
    return NULL;
}

static epicsThreadOSD * create_threadInfo(const char *name)
{
    epicsThreadOSD *pthreadInfo;

    /* sizeof(epicsThreadOSD) includes one byte for the '\0' */
    pthreadInfo = calloc(1,sizeof(*pthreadInfo) + strlen(name));
    if(!pthreadInfo)
        return NULL;
    pthreadInfo->suspendEvent = epicsEventCreateOnce(epicsEventEmpty);
    if(!pthreadInfo->suspendEvent){
        free(pthreadInfo);
        return NULL;
    }
    strcpy(pthreadInfo->name, name);
    return pthreadInfo;
}

static epicsThreadOSD * init_threadInfo(const char *name,
    unsigned int priority, unsigned int stackSize,
    EPICSTHREADFUNC funptr,void *parm)
{
    epicsThreadOSD *pthreadInfo;
    int status;

    pthreadInfo = create_threadInfo(name);
    if(!pthreadInfo)
        return NULL;
    pthreadInfo->createFunc = funptr;
    pthreadInfo->createArg = parm;
    status = pthread_attr_init(&pthreadInfo->attr);
    checkStatusOnce(status,"pthread_attr_init");
    if(status) return 0;
    status = pthread_attr_setdetachstate(
        &pthreadInfo->attr, PTHREAD_CREATE_DETACHED);
    checkStatusOnce(status,"pthread_attr_setdetachstate");
#if defined (_POSIX_THREAD_ATTR_STACKSIZE)
#if ! defined (OSITHREAD_USE_DEFAULT_STACK)
    status = pthread_attr_setstacksize( &pthreadInfo->attr,(size_t)stackSize);
    checkStatusOnce(status,"pthread_attr_setstacksize");
#endif /*OSITHREAD_USE_DEFAULT_STACK*/
#endif /*_POSIX_THREAD_ATTR_STACKSIZE*/
    status = pthread_attr_setscope(&pthreadInfo->attr,PTHREAD_SCOPE_PROCESS);
    if(errVerbose) checkStatusOnce(status,"pthread_attr_setscope");
    pthreadInfo->osiPriority = priority;
    return(pthreadInfo);
}

static void free_threadInfo(epicsThreadOSD *pthreadInfo)
{
    int status;

    if(pthreadInfo->isOnThreadList) {
      status = mutexLock(&listLock);
      checkStatusQuit(status,"pthread_mutex_lock","free_threadInfo");
      ellDelete(&pthreadList,&pthreadInfo->node);
      status = pthread_mutex_unlock(&listLock);
      checkStatusQuit(status,"pthread_mutex_unlock","free_threadInfo");
    }
    epicsEventDestroy(pthreadInfo->suspendEvent);
    status = pthread_attr_destroy(&pthreadInfo->attr);
    checkStatusQuit(status,"pthread_attr_destroy","free_threadInfo");
    free(pthreadInfo);
}

#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && _POSIX_THREAD_PRIORITY_SCHEDULING > 0
/*
 * The actually available range priority range (at least under linux)
 * may be restricted by resource limitations (but that is ignored
 * by sched_get_priority_max()). See bug #835138 which is fixed by
 * this code.
 */

static int try_pri(int pri, int policy)
{
struct sched_param  schedp;

    schedp.sched_priority = pri;
    return pthread_setschedparam(pthread_self(), policy, &schedp);
}

static void*
find_pri_range(void *arg)
{
priAvailable *prm = arg;
int           min = sched_get_priority_min(prm->policy);
int           max = sched_get_priority_max(prm->policy);
int           low, try;

    if ( -1 == min || -1 == max ) {
        /* something is very wrong; maintain old behavior
         * (warning message if sched_get_priority_xxx() fails
         * and use default policy's sched_priority [even if
         * that is likely to cause epicsThreadCreate to fail
         * because that priority is not suitable for SCHED_FIFO]).
         */
        prm->min_pri = prm->max_pri = -1;
        return 0;
    }


    if ( try_pri(min, prm->policy) ) {
        /* cannot create thread at minimum priority;
         * probably no permission to use SCHED_FIFO
         * at all. However, we still must return
         * a priority range accepted by the SCHED_FIFO
         * policy. Otherwise, epicsThreadCreate() cannot
         * detect the unsufficient permission (EPERM)
         * and fall back to a non-RT thread (because
         * pthread_attr_setschedparam would fail with
         * EINVAL due to the bad priority).
         */
        prm->min_pri = prm->max_pri = min;
        return 0;
    }


    /* Binary search through available priorities.
     * The actually available range may be restricted
     * by resource limitations (but that is ignored
     * by sched_get_priority_max() [linux]).
     */
    low = min;

    while ( low < max ) {
        try = (max+low)/2;
        if ( try_pri(try, prm->policy) ) {
            max = try;
        } else {
            low = try + 1;
        }
    }

    prm->min_pri = min;
    prm->max_pri = try_pri(max, prm->policy) ? max-1 : max;
    prm->ok = 1;

    return 0;
}

static void findPriorityRange(commonAttr *a_p)
{
priAvailable arg;
pthread_t    id;
void         *dummy;
int          status;

    arg.policy = a_p->schedPolicy;
    arg.ok = 0;

    status = pthread_create(&id, 0, find_pri_range, &arg);
    checkStatusOnceQuit(status, "pthread_create","findPriorityRange");

    status = pthread_join(id, &dummy);
    checkStatusOnceQuit(status, "pthread_join","findPriorityRange");
#if defined (__rtems__)
// We are using posix map osi 0-100 to posix 100-200
// see epicsThreadGetOsiPriorityValue(int ossPriority)
    a_p->minPriority = 100;
    a_p->maxPriority = 200;
#else
    a_p->minPriority = arg.min_pri;
    a_p->maxPriority = arg.max_pri;
#endif
    a_p->usePolicy = arg.ok;
}
#endif


static void once(void)
{
    int status;
    status = pthread_mutex_init(&onceLock,0);
    checkStatusOnceQuit(status,"pthread_mutex_init","once");
    status = pthread_mutex_init(&listLock,0);
    checkStatusOnceQuit(status,"pthread_mutex_init","once");
    pcommonAttr = calloc(1,sizeof(commonAttr));
    if(!pcommonAttr) checkStatusOnceQuit(errno,"calloc","once");
    status = pthread_attr_init(&pcommonAttr->attr);
    checkStatusOnceQuit(status,"pthread_attr_init","once");
    status = pthread_attr_setdetachstate(
        &pcommonAttr->attr, PTHREAD_CREATE_DETACHED);
    checkStatusOnce(status,"pthread_attr_setdetachstate");
    status = pthread_attr_setscope(&pcommonAttr->attr,PTHREAD_SCOPE_PROCESS);
    if(errVerbose) checkStatusOnce(status,"pthread_attr_setscope");

#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && _POSIX_THREAD_PRIORITY_SCHEDULING > 0
    status = pthread_attr_setschedpolicy(
        &pcommonAttr->attr,SCHED_FIFO);
    checkStatusOnce(status,"pthread_attr_setschedpolicy");
    status = pthread_attr_getschedpolicy(
        &pcommonAttr->attr,&pcommonAttr->schedPolicy);
    checkStatusOnce(status,"pthread_attr_getschedpolicy");
    status = pthread_attr_getschedparam(
        &pcommonAttr->attr,&pcommonAttr->schedParam);
    checkStatusOnce(status,"pthread_attr_getschedparam");

    findPriorityRange(pcommonAttr);

    if(pcommonAttr->maxPriority == -1) {
        pcommonAttr->maxPriority = pcommonAttr->schedParam.sched_priority;
        fprintf(stderr,"sched_get_priority_max failed set to %d\n",
            pcommonAttr->maxPriority);
    }
    if(pcommonAttr->minPriority == -1) {
        pcommonAttr->minPriority = pcommonAttr->schedParam.sched_priority;
        fprintf(stderr,"sched_get_priority_min failed set to %d\n",
            pcommonAttr->maxPriority);
    }

    if (errVerbose) {
#if defined(__rtems__)
        printk("LRT: min priority: %d max priority %d\n",
            pcommonAttr->minPriority, pcommonAttr->maxPriority);
#else
        fprintf(stderr, "LRT: min priority: %d max priority %d\n",
            pcommonAttr->minPriority, pcommonAttr->maxPriority);
#endif
    }

#else
    if(errVerbose) {
#if defined(__rtems__)
 printk("task priorities are not implemented\n");
#else
 fprintf(stderr,"task priorities are not implemented\n");
#endif
}
#endif /* _POSIX_THREAD_PRIORITY_SCHEDULING */
    int policy;
    struct sched_param param;
    status = pthread_getschedparam(pthread_self(), &policy, &param);
    checkStatusOnce(status, "pthread_getschedparam failed");
// param.sched_priority is still 2  bug or feature?
#if defined (__rtems__)
    param.sched_priority = 191; // iocsh prio 
#endif
    status = pthread_setschedparam(pthread_self(), policy, &param);
    checkStatusOnce(status, "pthread_setschedparam failed");
    status = pthread_getschedparam(pthread_self(), &policy, &param);
    checkStatusOnce(status, "pthread_getschedparam failed");

#if defined (__rtems__)
    tls_pthreadInfo = init_threadInfo("_main_",param.sched_priority-100,epicsThreadGetStackSize(epicsThreadStackSmall),0,0);
#else
    tls_pthreadInfo = init_threadInfo("_main_",0,epicsThreadGetStackSize(epicsThreadStackSmall),0,0);
#endif
    assert(tls_pthreadInfo!=NULL);
    tls_pthreadInfo->tid = pthread_self();
    status = mutexLock(&listLock);
    checkStatusOnceQuit(status,"pthread_mutex_lock","once");
    ellAdd(&pthreadList,&tls_pthreadInfo->node);
    tls_pthreadInfo->isOnThreadList = 1;
    status = pthread_mutex_unlock(&listLock);
    checkStatusOnceQuit(status,"pthread_mutex_unlock","once");
    status = atexit(epicsExitCallAtExits);
    checkStatusOnce(status,"atexit");
    osdThreadHooksRunMain(tls_pthreadInfo);
    epicsThreadInitOnceCalled = 1;
}

static void * start_routine(void *arg)
{
    int status;
    sigset_t blockAllSig;

    tls_pthreadInfo = (epicsThreadOSD *)arg;

    sigfillset(&blockAllSig);
    pthread_sigmask(SIG_SETMASK,&blockAllSig,NULL);
    status = mutexLock(&listLock);
    checkStatusQuit(status,"pthread_mutex_lock","start_routine");
    ellAdd(&pthreadList,&tls_pthreadInfo->node);
    tls_pthreadInfo->isOnThreadList = 1;
    status = pthread_mutex_unlock(&listLock);
    checkStatusQuit(status,"pthread_mutex_unlock","start_routine");
    osdThreadHooksRun(tls_pthreadInfo);

    (*tls_pthreadInfo->createFunc)(tls_pthreadInfo->createArg);

    epicsExitCallAtThreadExits ();
    free_threadInfo(tls_pthreadInfo);
    return(0);
}

static void epicsThreadInit(void)
{
    static pthread_once_t once_control = PTHREAD_ONCE_INIT;
    int status = pthread_once(&once_control,once);
    checkStatusQuit(status,"pthread_once","epicsThreadInit");
}

epicsShareFunc
void epicsThreadRealtimeLock(void)
{
#if !defined(__rtems__)
/* RTEMS defines _POSIX_MEMLOCK to 1 in features.h even if it is a non swaping OS and
   mlockall senseless*/
#if defined(_POSIX_MEMLOCK) && _POSIX_MEMLOCK > 0
    if (pcommonAttr->maxPriority > pcommonAttr->minPriority) {
        int status = mlockall(MCL_CURRENT | MCL_FUTURE);

        if (status) {
            fprintf(stderr, "epicsThreadRealtimeLock "
                "Warning: Unable to lock the virtual address space.\n"
                "VM page fautls may harm real-time performance.\n");
        }
    }
#endif
#endif /* not defined __rtems__ */
}

epicsShareFunc unsigned int epicsShareAPI epicsThreadGetStackSize (epicsThreadStackSizeClass stackSizeClass)
{
#if defined (OSITHREAD_USE_DEFAULT_STACK)
    return 0;
#elif defined(_POSIX_THREAD_ATTR_STACKSIZE) && _POSIX_THREAD_ATTR_STACKSIZE > 0
#if defined (__rtems__)
    #define STACK_SIZE(f) (f * 0x1000 * sizeof(void *))
#else
    #define STACK_SIZE(f) (f * 0x10000 * sizeof(void *))
#endif
    static const unsigned stackSizeTable[epicsThreadStackBig+1] = {
        STACK_SIZE(1), STACK_SIZE(2), STACK_SIZE(4)
    };
    if (stackSizeClass<epicsThreadStackSmall) {
        errlogPrintf("epicsThreadGetStackSize illegal argument (too small)");
        return stackSizeTable[epicsThreadStackBig];
    }

    if (stackSizeClass>epicsThreadStackBig) {
        errlogPrintf("epicsThreadGetStackSize illegal argument (too large)");
        return stackSizeTable[epicsThreadStackBig];
    }

    return stackSizeTable[stackSizeClass];
#else
    return 0;
#endif /*_POSIX_THREAD_ATTR_STACKSIZE*/
}

epicsShareFunc void epicsShareAPI epicsThreadOnce(epicsThreadOnceId *id, void (*func)(void *), void *arg)
{
    static struct epicsThreadOSD threadOnceComplete;
    #define EPICS_THREAD_ONCE_DONE &threadOnceComplete
    int status;

    epicsThreadInit();
    status = mutexLock(&onceLock);
    checkStatusOnceQuit(status,"pthread_mutex_lock", "epicsThreadOnce");

    if (*id != EPICS_THREAD_ONCE_DONE) {
        if (*id == EPICS_THREAD_ONCE_INIT) { /* first call */
            *id = epicsThreadGetIdSelf();    /* mark active */
            status = pthread_mutex_unlock(&onceLock);
            checkStatusOnceQuit(status,"pthread_mutex_unlock", "epicsThreadOnce");
            func(arg);
            status = mutexLock(&onceLock);
            checkStatusOnceQuit(status,"pthread_mutex_lock", "epicsThreadOnce");
            *id = EPICS_THREAD_ONCE_DONE;    /* mark done */
        } else if (*id == epicsThreadGetIdSelf()) {
            status = pthread_mutex_unlock(&onceLock);
            checkStatusOnceQuit(status,"pthread_mutex_unlock", "epicsThreadOnce");
            cantProceed("Recursive epicsThreadOnce() initialization\n");
        } else
            while (*id != EPICS_THREAD_ONCE_DONE) {
                /* Another thread is in the above func(arg) call. */
                status = pthread_mutex_unlock(&onceLock);
                checkStatusOnceQuit(status,"pthread_mutex_unlock", "epicsThreadOnce");
                epicsThreadSleep(epicsThreadSleepQuantum());
                status = mutexLock(&onceLock);
                checkStatusOnceQuit(status,"pthread_mutex_lock", "epicsThreadOnce");
            }
    }
    status = pthread_mutex_unlock(&onceLock);
    checkStatusOnceQuit(status,"pthread_mutex_unlock","epicsThreadOnce");
}

epicsShareFunc epicsThreadId epicsShareAPI epicsThreadCreate(const char *name,
    unsigned int priority, unsigned int stackSize,
    EPICSTHREADFUNC funptr,void *parm)
{
    epicsThreadOSD *pthreadInfo;
    int status;
    sigset_t blockAllSig, oldSig;

    epicsThreadInit();
    assert(pcommonAttr);
    sigfillset(&blockAllSig);
    pthread_sigmask(SIG_SETMASK,&blockAllSig,&oldSig);
    pthreadInfo = init_threadInfo(name,priority,stackSize,funptr,parm);
    if(pthreadInfo==0) return 0;
    pthreadInfo->isEpicsThread = 1;
    setSchedulingPolicy(pthreadInfo,SCHED_FIFO);
    pthreadInfo->isRealTimeScheduled = 1;
    status = pthread_create(&pthreadInfo->tid,&pthreadInfo->attr,
                start_routine,pthreadInfo);
    if(status==EPERM){
        /* Try again without SCHED_FIFO*/
        free_threadInfo(pthreadInfo);
        pthreadInfo = init_threadInfo(name,priority,stackSize,funptr,parm);
        if(pthreadInfo==0) return 0;
        pthreadInfo->isEpicsThread = 1;
        status = pthread_create(&pthreadInfo->tid,&pthreadInfo->attr,
                start_routine,pthreadInfo);
    }
    checkStatusOnce(status,"pthread_create");
    if(status) {
        free_threadInfo(pthreadInfo);
        return 0;
    }
    status = pthread_sigmask(SIG_SETMASK,&oldSig,NULL);
//? StatusOnce? because of errlog daemon ???
    checkStatusOnce(status,"pthread_sigmask");
    return(pthreadInfo);
}

/*
 * Create dummy context for threads not created by epicsThreadCreate().
 */
static void createImplicit(void)
{
    char name[64];
    pthread_t tid;

    tid = pthread_self();
    sprintf(name, "non-EPICS_%ld", (long)tid);
    tls_pthreadInfo = create_threadInfo(name);
    assert(tls_pthreadInfo);
    tls_pthreadInfo->tid = tid;
    tls_pthreadInfo->osiPriority = 0;

#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && _POSIX_THREAD_PRIORITY_SCHEDULING > 0
    {
    struct sched_param param;
    int policy;
    if(pthread_getschedparam(tid,&policy,&param) == 0)
        tls_pthreadInfo->osiPriority =
                 (param.sched_priority - pcommonAttr->minPriority) * 100.0 /
                    (pcommonAttr->maxPriority - pcommonAttr->minPriority);
    }
#endif /* _POSIX_THREAD_PRIORITY_SCHEDULING */
}

epicsShareFunc void epicsShareAPI epicsThreadSuspendSelf(void)
{
    epicsThreadInit();
    assert(tls_pthreadInfo);
    tls_pthreadInfo->isSuspended = 1;
    epicsEventWait(tls_pthreadInfo->suspendEvent);
}

epicsShareFunc void epicsShareAPI epicsThreadResume(epicsThreadOSD *pthreadInfo)
{
    assert(epicsThreadInitOnceCalled);
    pthreadInfo->isSuspended = 0;
    epicsEventSignal(pthreadInfo->suspendEvent);
}

epicsShareFunc void epicsShareAPI epicsThreadExitMain(void)
{

    epicsThreadInit();
    if(tls_pthreadInfo==NULL)
        createImplicit();
    if(tls_pthreadInfo->createFunc) {
        errlogPrintf("called from non-main thread\n");
        cantProceed("epicsThreadExitMain");
    }
    else {
    free_threadInfo(tls_pthreadInfo);
    pthread_exit(0);
    }
}

epicsShareFunc unsigned int epicsShareAPI epicsThreadGetPriority(epicsThreadId pthreadInfo)
{
    assert(epicsThreadInitOnceCalled);
    return(pthreadInfo->osiPriority);
}

epicsShareFunc unsigned int epicsShareAPI epicsThreadGetPrioritySelf(void)
{
    epicsThreadInit();
    return(epicsThreadGetPriority(epicsThreadGetIdSelf()));
}

epicsShareFunc void epicsShareAPI epicsThreadSetPriority(epicsThreadId pthreadInfo,unsigned int priority)
{
#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && _POSIX_THREAD_PRIORITY_SCHEDULING > 0
    int status;
#endif /* _POSIX_THREAD_PRIORITY_SCHEDULING */

    assert(epicsThreadInitOnceCalled);
    assert(pthreadInfo);
    if(!pthreadInfo->isEpicsThread) {
        fprintf(stderr,"epicsThreadSetPriority called by non epics thread\n");
        return;
    }
    pthreadInfo->osiPriority = priority;
    if(!pthreadInfo->isRealTimeScheduled) return;

#if defined (_POSIX_THREAD_PRIORITY_SCHEDULING) && _POSIX_THREAD_PRIORITY_SCHEDULING > 0
    if(!pcommonAttr->usePolicy) return;
    tls_pthreadInfo->schedParam.sched_priority = epicsThreadGetPosixPriority(pthreadInfo);
    status = pthread_attr_setschedparam(
        &pthreadInfo->attr,&pthreadInfo->schedParam);
    if(errVerbose) checkStatus(status,"pthread_attr_setschedparam");
    status = pthread_setschedparam(
        pthreadInfo->tid, pthreadInfo->schedPolicy, &pthreadInfo->schedParam);
    if(errVerbose) checkStatus(status,"pthread_setschedparam");
#endif /* _POSIX_THREAD_PRIORITY_SCHEDULING */
}

epicsShareFunc epicsThreadBooleanStatus epicsShareAPI epicsThreadHighestPriorityLevelBelow(
    unsigned int priority, unsigned *pPriorityJustBelow)
{
    unsigned newPriority = priority - 1;
#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && _POSIX_THREAD_PRIORITY_SCHEDULING > 0
    int diff;
    diff = pcommonAttr->maxPriority - pcommonAttr->minPriority;
    if(diff<0) diff = -diff;
    if(diff>1 && diff <100) newPriority -=  100/(diff+1);
#endif /* _POSIX_THREAD_PRIORITY_SCHEDULING */
    if (newPriority <= 99) {
        *pPriorityJustBelow = newPriority;
        return epicsThreadBooleanStatusSuccess;
    }
    return epicsThreadBooleanStatusFail;
}

epicsShareFunc epicsThreadBooleanStatus epicsShareAPI epicsThreadLowestPriorityLevelAbove(
    unsigned int priority, unsigned *pPriorityJustAbove)
{
    unsigned newPriority = priority + 1;

#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && _POSIX_THREAD_PRIORITY_SCHEDULING > 0
    int diff;
    diff = pcommonAttr->maxPriority - pcommonAttr->minPriority;
    if(diff<0) diff = -diff;
    if(diff>1 && diff <100) newPriority += 100/(diff+1);
#endif /* _POSIX_THREAD_PRIORITY_SCHEDULING */

    if (newPriority <= 99) {
        *pPriorityJustAbove = newPriority;
        return epicsThreadBooleanStatusSuccess;
    }
    return epicsThreadBooleanStatusFail;
}

epicsShareFunc int epicsShareAPI epicsThreadIsEqual(epicsThreadId p1, epicsThreadId p2)
{
    assert(epicsThreadInitOnceCalled);
    assert(p1);
    assert(p2);
    return(pthread_equal(p1->tid,p2->tid));
}

epicsShareFunc int epicsShareAPI epicsThreadIsSuspended(epicsThreadId pthreadInfo) {
    assert(epicsThreadInitOnceCalled);
    assert(pthreadInfo);
    return(pthreadInfo->isSuspended ? 1 : 0);
}

epicsShareFunc void epicsShareAPI epicsThreadSleep(double seconds)
{
    struct timespec delayTime;
    struct timespec remainingTime;
    double nanoseconds;

    if (seconds > 0) {
        delayTime.tv_sec = seconds;
        nanoseconds = (seconds - delayTime.tv_sec) *1e9;
        delayTime.tv_nsec = nanoseconds;
    }
    else {
        delayTime.tv_sec = 0;
        delayTime.tv_nsec = 0;
    }
    while (nanosleep(&delayTime, &remainingTime) == -1 &&
           errno == EINTR)
        delayTime = remainingTime;
}

epicsShareFunc epicsThreadId epicsShareAPI epicsThreadGetIdSelf(void) {

    epicsThreadInit();
    if(tls_pthreadInfo==NULL)
       createImplicit();
    return(tls_pthreadInfo);
}

epicsShareFunc pthread_t epicsThreadGetPosixThreadId ( epicsThreadId threadId )
{
    return threadId->tid;
}

epicsShareFunc epicsThreadId epicsShareAPI epicsThreadGetId(const char *name) {
    epicsThreadOSD *pthreadInfo;
    int status;

    assert(epicsThreadInitOnceCalled);
    status = mutexLock(&listLock);
    checkStatus(status,"pthread_mutex_lock epicsThreadGetId");
    if(status)
        return NULL;
    pthreadInfo=(epicsThreadOSD *)ellFirst(&pthreadList);
    while(pthreadInfo) {
    if(strcmp(name,pthreadInfo->name) == 0) break;
        pthreadInfo=(epicsThreadOSD *)ellNext(&pthreadInfo->node);
    }
    status = pthread_mutex_unlock(&listLock);
    checkStatus(status,"pthread_mutex_unlock epicsThreadGetId");

    return(pthreadInfo);
}

epicsShareFunc const char epicsShareAPI *epicsThreadGetNameSelf()
{

    epicsThreadInit();
    if(tls_pthreadInfo==NULL)
        createImplicit();
    return(tls_pthreadInfo->name);
}

epicsShareFunc void epicsShareAPI epicsThreadGetName(epicsThreadId pthreadInfo, char *name, size_t size)
{
    assert(epicsThreadInitOnceCalled);
    strncpy(name, pthreadInfo->name, size-1);
    name[size-1] = '\0';
}

epicsShareFunc void epicsThreadMap(EPICS_THREAD_HOOK_ROUTINE func)
{
    epicsThreadOSD *pthreadInfo;
    int status;

    epicsThreadInit();
    status = mutexLock(&listLock);
    checkStatus(status, "pthread_mutex_lock epicsThreadMap");
    if (status)
        return;
    pthreadInfo=(epicsThreadOSD *)ellFirst(&pthreadList);
    while (pthreadInfo) {
        func(pthreadInfo);
        pthreadInfo = (epicsThreadOSD *)ellNext(&pthreadInfo->node);
    }
    status = pthread_mutex_unlock(&listLock);
    checkStatus(status, "pthread_mutex_unlock epicsThreadMap");
}

epicsShareFunc void epicsShareAPI epicsThreadShowAll(unsigned int level)
{
    epicsThreadOSD *pthreadInfo;
    int status;

    epicsThreadInit();
    epicsThreadShow(0,level);
    status = mutexLock(&listLock);
    checkStatus(status,"pthread_mutex_lock epicsThreadShowAll");
    if(status)
        return;
    pthreadInfo=(epicsThreadOSD *)ellFirst(&pthreadList);
    while(pthreadInfo) {
        epicsThreadShowInfo(pthreadInfo,level);
        pthreadInfo=(epicsThreadOSD *)ellNext(&pthreadInfo->node);
    }
    status = pthread_mutex_unlock(&listLock);
    checkStatus(status,"pthread_mutex_unlock epicsThreadShowAll");
}

epicsShareFunc void epicsShareAPI epicsThreadShow(epicsThreadId showThread, unsigned int level)
{
    epicsThreadOSD *pthreadInfo;
    int status;
    int found = 0;

    epicsThreadInit();
    if(!showThread) {
        epicsThreadShowInfo(0,level);
        return;
    }
    status = mutexLock(&listLock);
    checkStatus(status,"pthread_mutex_lock epicsThreadShow");
    if(status)
        return;
    pthreadInfo=(epicsThreadOSD *)ellFirst(&pthreadList);
    while(pthreadInfo) {
        if (((epicsThreadId)pthreadInfo == showThread)
         || ((epicsThreadId)pthreadInfo->tid == showThread)) {
            found = 1;
            epicsThreadShowInfo(pthreadInfo,level);
        }
        pthreadInfo=(epicsThreadOSD *)ellNext(&pthreadInfo->node);
    }
    status = pthread_mutex_unlock(&listLock);
    checkStatus(status,"pthread_mutex_unlock epicsThreadShow");
    if(status) return;
    if (!found)
        printf("Thread %#lx (%lu) not found.\n", (unsigned long)showThread, (unsigned long)showThread);
}

epicsShareFunc epicsThreadPrivateId epicsShareAPI epicsThreadPrivateCreate(void)
{
    pthread_key_t *key;
    int status;

    epicsThreadInit();
    key = calloc(1,sizeof(pthread_key_t));
    if(!key)
        return NULL;
    status = pthread_key_create(key,0);
//used from errlogInit
    checkStatusOnce(status,"pthread_key_create epicsThreadPrivateCreate");
    if(status)
        return NULL;
    return((epicsThreadPrivateId)key);
}

epicsShareFunc void epicsShareAPI epicsThreadPrivateDelete(epicsThreadPrivateId id)
{
    pthread_key_t *key = (pthread_key_t *)id;
    int status;

    assert(epicsThreadInitOnceCalled);
    status = pthread_key_delete(*key);
    checkStatusQuit(status,"pthread_key_delete","epicsThreadPrivateDelete");
    free((void *)key);
}

epicsShareFunc void epicsShareAPI epicsThreadPrivateSet (epicsThreadPrivateId id, void *value)
{
    pthread_key_t *key = (pthread_key_t *)id;
    int status;

    assert(epicsThreadInitOnceCalled);
    if(errVerbose && !value)
        errlogPrintf("epicsThreadPrivateSet: setting value of 0\n");
    status = pthread_setspecific(*key,value);
    checkStatusQuit(status,"pthread_setspecific","epicsThreadPrivateSet");
}

epicsShareFunc void epicsShareAPI *epicsThreadPrivateGet(epicsThreadPrivateId id)
{
    pthread_key_t *key = (pthread_key_t *)id;

    assert(epicsThreadInitOnceCalled);
    return pthread_getspecific(*key);
}

epicsShareFunc double epicsShareAPI epicsThreadSleepQuantum ()
{
    double hz;
    hz = sysconf ( _SC_CLK_TCK );
    if(hz<0)
        return 0.0;
    return 1.0 / hz;
}

epicsShareFunc int epicsThreadGetCPUs(void)
{
    long ret;
#ifdef _SC_NPROCESSORS_ONLN
    ret = sysconf(_SC_NPROCESSORS_ONLN);
    if (ret > 0)
        return ret;
#endif
#ifdef _SC_NPROCESSORS_CONF
    ret = sysconf(_SC_NPROCESSORS_CONF);
    if (ret > 0)
        return ret;
#endif
    return 1;
}
