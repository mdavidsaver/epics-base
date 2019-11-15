/*************************************************************************\
* Copyright (c) 2014 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
#ifndef INC_epicsReadline_H
#define INC_epicsReadline_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libComAPI.h>
#include <stdio.h>

LIBCOM_API void * LIBCOMSTD_API epicsReadlineBegin (FILE *in);
LIBCOM_API char * LIBCOMSTD_API epicsReadline (const char *prompt, void *context);
LIBCOM_API void   LIBCOMSTD_API epicsReadlineEnd (void *context);

#ifdef __cplusplus
}
#endif

#endif /* INC_epicsReadline_H */
