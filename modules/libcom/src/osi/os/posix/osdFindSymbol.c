/*************************************************************************\
* Copyright (c) 2009 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/* osi/os/posix/osdFindSymbol.c */

#include <dlfcn.h>

#include "epicsFindSymbol.h"

LIBCOM_API void * epicsLoadLibrary(const char *name)
{
    return dlopen(name, RTLD_LAZY | RTLD_GLOBAL);
}

LIBCOM_API const char *epicsLoadError(void)
{
    return dlerror();
}

LIBCOM_API void * LIBCOMSTD_API epicsFindSymbol(const char *name)
{
    return dlsym(0, name);
}
