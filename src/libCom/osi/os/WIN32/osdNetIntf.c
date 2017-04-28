/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* Copyright (c) 2015 Brookhaven Science Associates as Operator of
*     Brookhaven National Lab.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/*
 *      WIN32 specific initialisation for bsd sockets,
 *      based on Chris Timossi's base/src/ca/windows_depend.c,
 *      and also further additions by Kay Kasemir when this was in
 *      dllmain.cc
 *
 *      7-1-97  -joh-
 *
 */

/*
 * ANSI C
 */
#include <stdio.h>
#include <stdlib.h>

/*
 * WIN32 specific
 */
#define VC_EXTRALEAN
#define STRICT
#include <winsock2.h>
#include <ws2tcpip.h>

/*
 * EPICS
 */
#define epicsExportSharedSymbols
#include "osiSock.h"
#include "errlog.h"
#include "epicsThread.h"
#include "epicsVersion.h"

static void osiLocalAddrOnce ( void *raw )
{
int ret = -1, status, foundlo = 0;
    SOCKET sock;
    unsigned nelem = 10, i;
    INTERFACE_INFO *info = NULL;
    DWORD				cbBytesReturned;

    /* only valid for winsock 2 and above */
    if (wsaMajorVersion() < 2 ) {
        fprintf(stderr, "Interface discovery not supported for winsock 1\n"
                        "Need to set EPICS_CA_AUTO_ADDR_LIST=NO\n");
        return ret;
    }

    sock = epicsSocketCreate(AF_INET, SOCK_DGRAM, 0);
    if(sock==INVALID_SOCKET)
        return ret;

    info = calloc(nelem, sizeof(*info));
    if(!info)
        goto cleanup;

    /* In future use SIO_GET_INTERFACE_LIST_EX to include IPv6 */

    status = WSAIoctl (sock, SIO_GET_INTERFACE_LIST,
                        NULL, 0,
                        (LPVOID)info, nelem*sizeof(*info),
                        &cbBytesReturned, NULL, NULL);

    if (status != 0 || cbBytesReturned == 0) {
        fprintf(stderr, "WSAIoctl SIO_GET_INTERFACE_LIST failed %d\n",WSAGetLastError());
        goto cleanup;
    }

    nelem = cbBytesReturned/sizeof(*info);

    for(i=0; i<nelem; i++)
    {
        unsigned int flags;
        osiInterfaceInfo *node = calloc(1, sizeof(*node));
        if(!node)
            goto cleanup;

        /* work around WS2 bug */
        if(info[i].iiAddress.AddressIn.sin_family==0)
           info[i].iiAddress.AddressIn.sin_family = AF_INET;

        if(info[i].iiAddress.AddressIn.sin_family!=AF_INET) {
            free(node);
            continue;
        }

        node->address.ia = info[i].iiAddress.AddressIn;
        node->netmask.ia = info[i].iiNetmask.AddressIn;
        node->endpoint.ia = info[i].iiBroadcastAddress.AddressIn;

        flags = info[i].iiFlags;

        if(flags&IFF_UP) node->up = 1;
        if(flags&IFF_BROADCAST) node->broadcast = 1;
        if(flags&IFF_MULTICAST) node->multicast = 1;
        if(flags&IFF_LOOPBACK) node->loopback = 1;
        /* BSD sockets have IFF_POINTOPOINT while winsock has IFF_POINTTOPOINT
         * Note the extra 'T'
         */
        if(flags&IFF_POINTTOPOINT) node->point2point = 1;

        if(node->broadcast && node->point2point) {
            errlogPrintf("Interface %u claims both broadcast and point to point,"
                         " which should not be possible.  Assuming broadcast only.",
                         i);
            node->point2point = 0;
        }

        if(node->loopback) foundlo = 1;
        ellAdd(pList, &node->node);
    }

    if(!foundlo) {
        /* sometimes the loopback isn't included (WINE+mingw) */
        osiInterfaceInfo *node = calloc(1, sizeof(*node));
        if(!node)
            goto cleanup;

        node->up = 1;
        node->loopback = 1;

        node->address.ia.sin_family = AF_INET;
        node->address.ia.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        node->address.ia.sin_port = 0;

        node->netmask.ia.sin_family = AF_INET;
        node->netmask.ia.sin_addr.s_addr = htonl(0xff000000);
        node->netmask.ia.sin_port = 0;

        ellInsert(pList, NULL, &node->node);
    }

    ret = 0;
cleanup:
    if(ret)
        ellFree(pList);
    free(info);
    epicsSocketDestroy(sock);
    return ret;
}
