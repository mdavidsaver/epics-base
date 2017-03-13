/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* Copyright (c) 2017 Fritz-Haber-Institut der Max-Planck-Gesellschaft
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

/*
 *     Heinz Junkes
 *     Adapted to rtems4.12
 */

#include <rtems.h>
#include <rtems/malloc.h>
#include <rtems/score/heap.h>
#define epicsExportSharedSymbols
#include "osiPoolStatus.h"

/*
 * osiSufficentSpaceInPool ()
 * 
 */
epicsShareFunc int epicsShareAPI osiSufficentSpaceInPool ( size_t contiguousBlockSize )
{
    unsigned long n;
    Heap_Information_block info;

    malloc_info( &info );
    n = info.Stats.size - (unsigned long)(info.Stats.lifetime_allocated - info.Stats.lifetime_freed);
    return (n > (50000 + contiguousBlockSize));
}
