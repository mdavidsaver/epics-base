/*************************************************************************\
* Copyright (c) 2019 Michael Davidsaver
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#if !defined(_WIN32) && !defined(vxWorks)

#include <syslog.h>

#define epicsExportSharedSymbols
#include <errlog.h>
#include <logClient.h>
#include <envDefs.h>

/* logClient.c */
extern char* logClientPrefix;

static
void err2sys(void *unused, const char *message)
{
    (void)unused;
    syslog(LOG_INFO, "%s", message);
}

void errlogToSyslog(void)
{
    const char *ident;
    iocLogPrefix(NULL);
    ident = logClientPrefix;

    if(!ident || !ident[0])
        return;

    openlog(ident, 0, LOG_USER);

    errlogAddListener(err2sys, 0);
    syslog(LOG_DEBUG, "Syslog connected");
}

#else /* !_WIN32 && !vxWorks */

#define epicsExportSharedSymbols
#include <errlog.h>

void errlogToSyslog(void) {}

#endif /* _WIN32 && !vxWorks */
