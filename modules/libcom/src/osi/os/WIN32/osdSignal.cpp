/*************************************************************************\
 * Copyright (c) 2002 The University of Chicago, as Operator of Argonne
 *     National Laboratory.
 * Copyright (c) 2002 The Regents of the University of California, as
 *     Operator of Los Alamos National Laboratory.
 * EPICS BASE Versions 3.13.7
 * and higher are distributed subject to a Software License Agreement found
 * in file LICENSE that is included with this distribution. 
\*************************************************************************/

#include "epicsSignal.h"

/*
 * NOOP
 */
LIBCOM_API void LIBCOMSTD_API epicsSignalInstallSigHupIgnore ( void ) {}
LIBCOM_API void LIBCOMSTD_API epicsSignalInstallSigPipeIgnore ( void ) {}
LIBCOM_API void LIBCOMSTD_API epicsSignalInstallSigAlarmIgnore ( void ) {}
LIBCOM_API void LIBCOMSTD_API epicsSignalRaiseSigAlarm ( struct epicsThreadOSD * /* threadid */ ) {}

