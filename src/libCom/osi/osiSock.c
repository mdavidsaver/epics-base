/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* Copyright (c) 2015 Brookhaven Science Associates as Operator of
*     Brookhaven National Lab.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

/*
 *      socket support library generic code
 *
 *      7-1-97  -joh-
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define epicsExportSharedSymbols
#include "epicsAssert.h"
#include "epicsSignal.h"
#include "epicsStdio.h"
#include "dbDefs.h"
#include "errlog.h"
#include "osiSock.h"

#define nDigitsDottedIP 4u
#define chunkSize 8u

#define makeMask(NBITS) ( ( 1u << ( (unsigned) NBITS) ) - 1u )

/*
 * sockAddrAreIdentical() 
 * (returns true if addresses are identical)
 */
int epicsShareAPI sockAddrAreIdentical 
			( const osiSockAddr *plhs, const osiSockAddr *prhs )
{
    int match;

    if ( plhs->sa.sa_family != prhs->sa.sa_family ) {
        match = 0;
    }
    else if ( plhs->sa.sa_family != AF_INET ) {
        match = 0;
    }
    else if ( plhs->ia.sin_addr.s_addr != prhs->ia.sin_addr.s_addr ) {
        match = 0;
    }
    else if ( plhs->ia.sin_port != prhs->ia.sin_port ) { 
        match = 0;
    }
    else {
        match = 1;
    }
    return match;
}

/*
 * sockAddrToA() 
 * (convert socket address to ASCII host name)
 */
unsigned epicsShareAPI sockAddrToA ( 
    const struct sockaddr * paddr, char * pBuf, unsigned bufSize )
{
	if ( bufSize < 1 ) {
		return 0;
	}

	if ( paddr->sa_family != AF_INET ) {
        static const char * pErrStr = "<Ukn Addr Type>";
        unsigned len = strlen ( pErrStr );
        if ( len < bufSize ) {
            strcpy ( pBuf, pErrStr );
            return len;
        }
        else {
		    strncpy ( pBuf, "<Ukn Addr Type>", bufSize-1 );
		    pBuf[bufSize-1] = '\0';
            return bufSize-1;
        }
	}
	else {
        const struct sockaddr_in * paddr_in = 
            (const struct sockaddr_in *) paddr;
        return ipAddrToA ( paddr_in, pBuf, bufSize );
	}
}

/*
 * ipAddrToA() 
 * (convert IP address to ASCII host name)
 */
unsigned epicsShareAPI ipAddrToA ( 
    const struct sockaddr_in * paddr, char * pBuf, unsigned bufSize )
{
	unsigned len = ipAddrToHostName ( 
        & paddr->sin_addr, pBuf, bufSize );
	if ( len == 0 ) {
        len = ipAddrToDottedIP ( paddr, pBuf, bufSize );
	}
    else {
        unsigned reducedSize = bufSize - len;
        int status = epicsSnprintf ( 
                        &pBuf[len], reducedSize, ":%hu", 
                        ntohs (paddr->sin_port) );
        if ( status > 0 ) {
            unsigned portSize = (unsigned) status;
            if ( portSize < reducedSize ) {
                len += portSize;
            }
        }
    }
    return len;
}

/*
 * sockAddrToDottedIP () 
 */
unsigned epicsShareAPI sockAddrToDottedIP ( 
    const struct sockaddr * paddr, char * pBuf, unsigned bufSize )
{
	if ( paddr->sa_family != AF_INET ) {
        const char * pErrStr = "<Ukn Addr Type>";
        unsigned errStrLen = strlen ( pErrStr );
        if ( errStrLen < bufSize ) {
            strcpy ( pBuf, pErrStr );
            return errStrLen;
        }
        else {
            unsigned reducedSize = bufSize - 1u;
            strncpy ( pBuf, pErrStr, reducedSize );
		    pBuf[reducedSize] = '\0';
            return reducedSize;
        }
	}
	else {
        const struct sockaddr_in *paddr_in = ( const struct sockaddr_in * ) paddr;
        return ipAddrToDottedIP ( paddr_in, pBuf, bufSize );
    }
}

/*
 * ipAddrToDottedIP () 
 */
unsigned epicsShareAPI ipAddrToDottedIP ( 
    const struct sockaddr_in *paddr, char *pBuf, unsigned bufSize )
{
    static const char * pErrStr = "<IPA>";
    unsigned chunk[nDigitsDottedIP];
    unsigned addr = ntohl ( paddr->sin_addr.s_addr );
    unsigned strLen;
    unsigned i;
    int status;

    if ( bufSize == 0u ) {
        return 0u;
    }

    for ( i = 0; i < nDigitsDottedIP; i++ ) {
        chunk[i] = addr & makeMask ( chunkSize );
        addr >>= chunkSize;
    }

    /*
     * inet_ntoa() isnt used because it isnt thread safe
     * (and the replacements are not standardized)
     */
    status = epicsSnprintf ( 
                pBuf, bufSize, "%u.%u.%u.%u:%hu", 
                chunk[3], chunk[2], chunk[1], chunk[0], 
                ntohs ( paddr->sin_port ) );
    if ( status > 0 ) {
        strLen = ( unsigned ) status;
        if ( strLen < bufSize - 1 ) {
            return strLen;
        }
    }
    strLen = strlen ( pErrStr );
    if ( strLen < bufSize ) {
        strcpy ( pBuf, pErrStr );
        return strLen;
    }
    else {
        strncpy ( pBuf, pErrStr, bufSize );
	    pBuf[bufSize-1] = '\0';
        return bufSize - 1u;
    }
}

epicsShareFunc void osiFreeInterfaceInfo(osiInterfaceInfo *pinfo)
{
    free(pinfo);
}

/*
 * osiSockDiscoverBroadcastAddresses ()
 */
epicsShareFunc void epicsShareAPI osiSockDiscoverBroadcastAddresses
     (ELLLIST *pList, SOCKET socket, const osiSockAddr *pMatchAddr)
{
    ELLLIST infolist = ELLLIST_INIT;
    ELLNODE *cur;

    if ( pMatchAddr->sa.sa_family == AF_INET  ) {
        if ( pMatchAddr->ia.sin_addr.s_addr == htonl (INADDR_LOOPBACK) ) {
            osiSockAddrNode *pNewNode = calloc (1, sizeof (*pNewNode) );
            if ( pNewNode == NULL ) {
                errlogPrintf ( "osiSockDiscoverBroadcastAddresses(): no memory available for configuration\n" );
                return;
            }
            pNewNode->addr.ia.sin_family = AF_INET;
            pNewNode->addr.ia.sin_port = htons ( 0 );
            pNewNode->addr.ia.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
            ellAdd ( pList, &pNewNode->node );
            return;
        }
    } else if ( pMatchAddr->sa.sa_family != AF_UNSPEC ) {
        errlogPrintf("osiSockDiscoverBroadcastAddresses(): match address must be AF_INET or AF_UNSPEC.");
        return;
    }

    if(osiGetInterfaceInfo(&infolist, 0)) {
        errlogPrintf ("osiSockDiscoverBroadcastAddresses(): unable to fetch network interface configuration\n");
        return;
    }

    if(ellCount(&infolist)==0) {
        errlogPrintf ("osiSockDiscoverBroadcastAddresses(): no network interfaces found\n");
    }

    for(cur=ellFirst(&infolist); cur; cur=ellNext(cur))
    {
        osiSockAddrNode *pNewNode;
        osiInterfaceInfo *info = CONTAINER(cur, osiInterfaceInfo, node);

        if(info->address.sa.sa_family!=AF_INET || !info->broadcast)
            continue;

        pNewNode = calloc(1, sizeof(*pNewNode));
        if(!pNewNode)
            break;

        if(pMatchAddr->ia.sin_family==AF_INET &&
           pMatchAddr->ia.sin_addr.s_addr != htonl(INADDR_ANY) &&
           pMatchAddr->ia.sin_addr.s_addr != info->address.ia.sin_addr.s_addr)
        {
            free(pNewNode);
            continue;
        }

        pNewNode->addr.ia = info->endpoint.ia;

        ellAdd(pList, &pNewNode->node);
    }

    ellFree2(&infolist, (FREEFUNC)osiFreeInterfaceInfo);
}

/*
 * osiLocalAddr ()
 */
epicsShareFunc osiSockAddr epicsShareAPI osiLocalAddr (SOCKET socket)
{
    static osiSockAddr result;
    static int init;

    if(!init) {
        ELLLIST infolist = ELLLIST_INIT;
        ELLNODE *cur;
        osiSockAddr      addr;
        int found = 0;

        memset ( (void *) &addr, '\0', sizeof ( addr ) );
        addr.sa.sa_family = AF_UNSPEC;

        if(osiGetInterfaceInfo(&infolist, 0)) {
            errlogPrintf ("osiLocalAddr(): unable to fetch network interface configuration\n");

        } else {

            for(cur=ellFirst(&infolist); cur; cur=ellNext(cur))
            {
                osiInterfaceInfo *info = CONTAINER(cur, osiInterfaceInfo, node);

                if(info->address.sa.sa_family!=AF_INET || !info->up || info->loopback)
                    continue;

                addr.ia = info->address.ia;
                found = 1;
            }

            ellFree(&infolist);
        }

        if(!found) {
            addr.ia.sin_family = AF_INET;
            addr.ia.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            addr.ia.sin_port = 0;
        }

        result = addr;
        init = 1;
    }

    return result;
}

epicsShareFunc
int osiGetInterfaceInfoSingle(const osiSockAddr *paddr, osiInterfaceInfo **presult, unsigned flags)
{
    ELLLIST infolist = ELLLIST_INIT;
    ELLNODE *cur;
    int found = 0;

    if(paddr->sa.sa_family!=AF_INET)
        return -1;

    if(osiGetInterfaceInfo(&infolist, flags)) {
        errlogPrintf ("osiGetInterfaceInfoSingle(): unable to fetch network interface configuration\n");
        return -1;
    }

    for(cur=ellFirst(&infolist); cur; cur=ellNext(cur))
    {
        osiInterfaceInfo *info = CONTAINER(cur, osiInterfaceInfo, node);

        if(info->address.ia.sin_addr.s_addr==paddr->ia.sin_addr.s_addr) {
            if(presult) {
                *presult = info;
                ellDelete(&infolist, cur);
                // caller now responsible to free
            }
            found = 1;
            break;
        }
    }

    ellFree2(&infolist, (FREEFUNC)osiFreeInterfaceInfo);
    return found ? 0 : -1;
}

