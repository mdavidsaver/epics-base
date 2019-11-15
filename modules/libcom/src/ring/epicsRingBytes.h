/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* Copyright (c) 2012 ITER Organization.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/*
 * Author:  Marty Kraimer Date:    15JUL99
 *          Eric Norum
 *          Ralph Lange <Ralph.Lange@gmx.de>
 */

#ifndef INCepicsRingBytesh
#define INCepicsRingBytesh

#ifdef __cplusplus
extern "C" {
#endif

#include "libComAPI.h"

typedef void *epicsRingBytesId;
typedef void const *epicsRingBytesIdConst;

LIBCOM_API epicsRingBytesId  LIBCOMSTD_API epicsRingBytesCreate(int nbytes);
/* Same, but secured by a spinlock */
LIBCOM_API epicsRingBytesId  LIBCOMSTD_API epicsRingBytesLockedCreate(int nbytes);
LIBCOM_API void LIBCOMSTD_API epicsRingBytesDelete(epicsRingBytesId id);
LIBCOM_API int  LIBCOMSTD_API epicsRingBytesGet(
    epicsRingBytesId id, char *value,int nbytes);
LIBCOM_API int  LIBCOMSTD_API epicsRingBytesPut(
    epicsRingBytesId id, char *value,int nbytes);
LIBCOM_API void LIBCOMSTD_API epicsRingBytesFlush(epicsRingBytesId id);
LIBCOM_API int  LIBCOMSTD_API epicsRingBytesFreeBytes(epicsRingBytesId id);
LIBCOM_API int  LIBCOMSTD_API epicsRingBytesUsedBytes(epicsRingBytesId id);
LIBCOM_API int  LIBCOMSTD_API epicsRingBytesSize(epicsRingBytesId id);
LIBCOM_API int  LIBCOMSTD_API epicsRingBytesIsEmpty(epicsRingBytesId id);
LIBCOM_API int  LIBCOMSTD_API epicsRingBytesIsFull(epicsRingBytesId id);
LIBCOM_API int  LIBCOMSTD_API epicsRingBytesHighWaterMark(epicsRingBytesIdConst id);
LIBCOM_API void LIBCOMSTD_API epicsRingBytesResetHighWaterMark(epicsRingBytesId id);

#ifdef __cplusplus
}
#endif

/* NOTES
    If there is only one writer it is not necessary to lock for put
    If there is a single reader it is not necessary to lock for puts

    epicsRingBytesLocked uses a spinlock.
*/

#endif /* INCepicsRingBytesh */
