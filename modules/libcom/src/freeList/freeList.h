/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* Author:  Marty Kraimer Date:    04-19-94	*/

#ifndef INCfreeListh
#define INCfreeListh

#include <stddef.h>
#include "libComAPI.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBCOM_API void LIBCOMSTD_API freeListInitPvt(void **ppvt,int size,int nmalloc);
LIBCOM_API void * LIBCOMSTD_API freeListCalloc(void *pvt);
LIBCOM_API void * LIBCOMSTD_API freeListMalloc(void *pvt);
LIBCOM_API void LIBCOMSTD_API freeListFree(void *pvt,void*pmem);
LIBCOM_API void LIBCOMSTD_API freeListCleanup(void *pvt);
LIBCOM_API size_t LIBCOMSTD_API freeListItemsAvail(void *pvt);

#ifdef __cplusplus
}
#endif

#endif /*INCfreeListh*/
