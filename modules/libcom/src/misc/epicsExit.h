/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*epicsExit.h*/
#ifndef epicsExith
#define epicsExith
#include <libComAPI.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*epicsExitFunc)(void *arg);

LIBCOM_API void epicsExit(int status);
LIBCOM_API void epicsExitLater(int status);
LIBCOM_API void epicsExitCallAtExits(void);
LIBCOM_API int epicsAtExit3(epicsExitFunc func, void *arg, const char* name);
#define epicsAtExit(F,A) epicsAtExit3(F,A,#F)

LIBCOM_API void epicsExitCallAtThreadExits(void);
LIBCOM_API int epicsAtThreadExit(epicsExitFunc func, void *arg);


#ifdef __cplusplus
}
#endif

#endif /*epicsExith*/
