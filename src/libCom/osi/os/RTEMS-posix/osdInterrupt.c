/*************************************************************************\
* Copyright (c) 2009 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* osi/default/osdInterrupt.c */

/* Author:  Marty Kraimer Date:    15JUL99 */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#define epicsExportSharedSymbols
#include "epicsMutex.h"
#include "epicsThread.h"
#include "cantProceed.h"
#include "errlog.h"
#include "epicsInterrupt.h"

#include <rtems/bspIo.h>
#include <rtems.h>

epicsShareFunc int epicsInterruptLock()
{
    rtems_interrupt_level level;

    rtems_interrupt_disable (level);
    return level;
}

epicsShareFunc void epicsInterruptUnlock(int key)
{
    rtems_interrupt_level level = key;

    rtems_interrupt_enable (level);
}

epicsShareFunc int epicsInterruptIsInterruptContext()
{
        return rtems_interrupt_is_in_progress ();
}

epicsShareFunc void epicsInterruptContextMessage(const char *message)
{
    printk("%s", message);
}







