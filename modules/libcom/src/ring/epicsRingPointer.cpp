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
 * Author:  Marty Kraimer Date:    13OCT2000
 *          Ralph Lange <Ralph.Lange@gmx.de>
 */


#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include "epicsRingPointer.h"
typedef epicsRingPointer<void> voidPointer;


LIBCOM_API epicsRingPointerId  LIBCOMSTD_API epicsRingPointerCreate(int size)
{
    voidPointer *pvoidPointer = new voidPointer(size, false);
    return(reinterpret_cast<void *>(pvoidPointer));
}

LIBCOM_API epicsRingPointerId  LIBCOMSTD_API epicsRingPointerLockedCreate(int size)
{
    voidPointer *pvoidPointer = new voidPointer(size, true);
    return(reinterpret_cast<void *>(pvoidPointer));
}

LIBCOM_API void LIBCOMSTD_API epicsRingPointerDelete(epicsRingPointerId id)
{
    voidPointer *pvoidPointer = reinterpret_cast<voidPointer*>(id);
    delete pvoidPointer;
}

LIBCOM_API void* LIBCOMSTD_API epicsRingPointerPop(epicsRingPointerId id)
{
    voidPointer *pvoidPointer = reinterpret_cast<voidPointer*>(id);
    return pvoidPointer->pop();
}

LIBCOM_API int LIBCOMSTD_API epicsRingPointerPush(epicsRingPointerId id, void *p)
{
    voidPointer *pvoidPointer = reinterpret_cast<voidPointer*>(id);
    return((pvoidPointer->push(p) ? 1 : 0));
}

LIBCOM_API void LIBCOMSTD_API epicsRingPointerFlush(epicsRingPointerId id)
{
    voidPointer *pvoidPointer = reinterpret_cast<voidPointer*>(id);
    pvoidPointer->flush();
}

LIBCOM_API int LIBCOMSTD_API epicsRingPointerGetFree(epicsRingPointerId id)
{
    voidPointer *pvoidPointer = reinterpret_cast<voidPointer*>(id);
    return(pvoidPointer->getFree());
}

LIBCOM_API int LIBCOMSTD_API epicsRingPointerGetUsed(epicsRingPointerId id)
{
    voidPointer *pvoidPointer = reinterpret_cast<voidPointer*>(id);
    return(pvoidPointer->getUsed());
}

LIBCOM_API int LIBCOMSTD_API epicsRingPointerGetSize(epicsRingPointerId id)
{
    voidPointer *pvoidPointer = reinterpret_cast<voidPointer*>(id);
    return(pvoidPointer->getSize());
}

LIBCOM_API int LIBCOMSTD_API epicsRingPointerIsEmpty(epicsRingPointerId id)
{
    voidPointer *pvoidPointer = reinterpret_cast<voidPointer*>(id);
    return((pvoidPointer->isEmpty()) ? 1 : 0);
}

LIBCOM_API int LIBCOMSTD_API epicsRingPointerIsFull(epicsRingPointerId id)
{
    voidPointer *pvoidPointer = reinterpret_cast<voidPointer*>(id);
    return((pvoidPointer->isFull()) ? 1 : 0);
}

LIBCOM_API int LIBCOMSTD_API epicsRingPointerGetHighWaterMark(epicsRingPointerIdConst id)
{
    voidPointer const *pvoidPointer = reinterpret_cast<voidPointer const*>(id);
    return(pvoidPointer->getHighWaterMark());
}

LIBCOM_API void LIBCOMSTD_API epicsRingPointerResetHighWaterMark(epicsRingPointerId id)
{
    voidPointer *pvoidPointer = reinterpret_cast<voidPointer*>(id);
    pvoidPointer->resetHighWaterMark();
}
