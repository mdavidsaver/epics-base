/*************************************************************************\
* Copyright (c) 2007 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/*
 * Definitions for macro substitution library (macLib)
 *
 * William Lupton, W. M. Keck Observatory
 */

#ifndef INCmacLibH
#define INCmacLibH

/*
 * EPICS include files needed by this file
 */
#include "ellLib.h"
#include "libComAPI.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Maximum size of macro name or value string (simpler to make fixed)
 */
#define MAC_SIZE 256

/*
 * Macro substitution context. One of these contexts is allocated each time
 * macCreateHandle() is called
 */
typedef struct {
    long        magic;          /* magic number (used for authentication) */
    int         dirty;          /* values need expanding from raw values? */
    int         level;          /* scoping level */
    int         debug;          /* debugging level */
    ELLLIST     list;           /* macro name / value list */
    int         flags;          /* operating mode flags */
} MAC_HANDLE;

/*
 * Function prototypes (core library)
 */
LIBCOM_API long             /* 0 = OK; <0 = ERROR */
LIBCOMSTD_API macCreateHandle(
    MAC_HANDLE  **handle,       /* address of variable to receive pointer */
                                /* to new macro substitution context */

    const char * pairs[]        /* pointer to NULL-terminated array of */
                                /* {name,value} pair strings; a NULL */
                                /* value implies undefined; a NULL */
                                /* argument implies no macros */
);

LIBCOM_API void
LIBCOMSTD_API macSuppressWarning(
    MAC_HANDLE  *handle,        /* opaque handle */

    int         falseTrue       /*0 means issue, 1 means suppress*/
);

LIBCOM_API long             /* strlen(dest), <0 if any macros are */
                                /* undefined */
LIBCOMSTD_API macExpandString(
    MAC_HANDLE  *handle,        /* opaque handle */

    const char  *src,           /* source string */

    char        *dest,          /* destination string */

    long        capacity        /* capacity of destination buffer (dest) */
);


LIBCOM_API long             /* strlen(value) */
LIBCOMSTD_API macPutValue(
    MAC_HANDLE  *handle,        /* opaque handle */

    const char  *name,          /* macro name */

    const char  *value          /* macro value */
);

LIBCOM_API long             /* strlen(value), <0 if undefined */
LIBCOMSTD_API macGetValue(
    MAC_HANDLE  *handle,        /* opaque handle */

    const char  *name,          /* macro name or reference */

    char        *value,         /* string to receive macro value or name */
                                /* argument if macro is undefined */

    long        capacity        /* capacity of destination buffer (value) */
);

LIBCOM_API long             /* 0 = OK; <0 = ERROR */
LIBCOMSTD_API macDeleteHandle(
    MAC_HANDLE  *handle         /* opaque handle */
);

LIBCOM_API long             /* 0 = OK; <0 = ERROR */
LIBCOMSTD_API macPushScope(
    MAC_HANDLE  *handle         /* opaque handle */
);

LIBCOM_API long             /* 0 = OK; <0 = ERROR */
LIBCOMSTD_API macPopScope(
    MAC_HANDLE  *handle         /* opaque handle */
);

LIBCOM_API long             /* 0 = OK; <0 = ERROR */
LIBCOMSTD_API macReportMacros(
    MAC_HANDLE  *handle         /* opaque handle */
);

/*
 * Function prototypes (utility library)
 */
LIBCOM_API long             /* #defns encountered; <0 = ERROR */
LIBCOMSTD_API macParseDefns(
    MAC_HANDLE  *handle,        /* opaque handle; can be NULL if default */
                                /* special characters are to be used */

    const char  *defns,         /* macro definitions in "a=xxx,b=yyy" */
                                /* format */

    char        **pairs[]       /* address of variable to receive pointer */
                                /* to NULL-terminated array of {name, */
                                /* value} pair strings; all storage is */
                                /* allocated contiguously */
);

LIBCOM_API long             /* #macros defined; <0 = ERROR */
LIBCOMSTD_API macInstallMacros(
    MAC_HANDLE  *handle,        /* opaque handle */

    char        *pairs[]        /* pointer to NULL-terminated array of */
                                /* {name,value} pair strings; a NULL */
                                /* value implies undefined; a NULL */
                                /* argument implies no macros */
);

LIBCOM_API char *           /* expanded string; NULL if any undefined macros */
LIBCOMSTD_API macEnvExpand(
    const char *str             /* string to be expanded */
);

LIBCOM_API char *           /* expanded string; NULL if any undefined macros */
LIBCOMSTD_API macDefExpand(
    const char *str,            /* string to be expanded */
    MAC_HANDLE *macros          /* opaque handle; can be NULL if default */
                                /* special characters are to be used */
);

#ifdef __cplusplus
}
#endif

#endif /*INCmacLibH*/
