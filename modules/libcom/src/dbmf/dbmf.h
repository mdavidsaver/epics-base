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
 * Author: Jim Kowalkowski and Marty Kraimer
 * Date:   4/97
 *
 * A library to manage storage that is allocated and then freed.
 */
#ifndef DBMF_H
#define DBMF_H

#include <stdlib.h>
#include "libComAPI.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBCOM_API int dbmfInit(size_t size, int chunkItems);
LIBCOM_API void * dbmfMalloc(size_t bytes);
LIBCOM_API char * dbmfStrdup(const char *str);
LIBCOM_API char * dbmfStrndup(const char *str, size_t len);
LIBCOM_API char * dbmfStrcat3(const char *lhs, const char *mid,
    const char *rhs);
LIBCOM_API void dbmfFree(void *bytes);
LIBCOM_API void dbmfFreeChunks(void);
LIBCOM_API int dbmfShow(int level);

/* Rules:
 * 1) Size is always made a multiple of 8.
 * 2) if dbmfInit is not called before one of the other routines then it
 *    is automatically called with size=64 and chunkItems=10
 * 3) These routines should only be used to allocate storage that will
 *    shortly thereafter be freed. 
 * 4) dbmfFreeChunks can only free chunks that contain only free items
*/

#ifdef __cplusplus
}
#endif

#endif
