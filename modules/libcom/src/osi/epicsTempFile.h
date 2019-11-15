/*************************************************************************\
* Copyright (c) 2012 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

/* epicsTempFile.h */

#ifndef INC_epicsTempFile_H
#define INC_epicsTempFile_H

#include <stdio.h>

#include "libComAPI.h"

#ifdef  __cplusplus
extern "C" {
#endif

LIBCOM_API FILE * LIBCOMSTD_API epicsTempFile(void);

#ifdef  __cplusplus
}
#endif

#endif /* INC_epicsTempFile_H */
