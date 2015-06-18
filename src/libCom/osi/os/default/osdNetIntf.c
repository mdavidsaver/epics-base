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

/* $Revision-Id$ */
/*
 *      Author:		Jeff Hill 
 *      Date:       04-05-94 
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define epicsExportSharedSymbols
#include "dbDefs.h"
#include "osiSock.h"
#include "epicsAssert.h"
#include "errlog.h"

#ifdef DEBUG
#   define ifDepenDebugPrintf(argsInParen) printf argsInParen
#else
#   define ifDepenDebugPrintf(argsInParen)
#endif


#ifndef USE_IFADDRS
/*
 * Determine the size of an ifreq structure
 * Made difficult by the fact that addresses larger than the structure
 * size may be returned from the kernel.
 */
static size_t ifreqSize ( struct ifreq *pifreq )
{
    size_t        size;

    size = ifreq_size ( pifreq );
    if ( size < sizeof ( *pifreq ) ) {
	    size = sizeof ( *pifreq );
    }
    return size;
}

/*
 * Move to the next ifreq structure
 */
static struct ifreq * ifreqNext ( struct ifreq *pifreq )
{
    struct ifreq *ifr;

    ifr = ( struct ifreq * )( ifreqSize (pifreq) + ( char * ) pifreq );
    ifDepenDebugPrintf( ("ifreqNext() pifreq 0x%08x, size 0x%08x, ifr 0x%08x\n", pifreq, ifreqSize (pifreq), ifr) );
    return ifr;
}
#endif /* USE_IFADDRS */

unsigned maxreq = 10*sizeof(struct ifreq); /* TODO: detect overflow */

#ifdef USE_IFADDRS

epicsShareFunc int osiGetInterfaceInfo(ELLLIST *pList)
{
    int ret = -1;
    struct ifaddrs *addrs = NULL, *cur;

    ellFree(pList);

    if(getifaddrs(&addrs))
        goto cleanup;

    for(cur=addrs; cur; cur=cur?cur->ifa_next:NULL)
    {
        unsigned int flags;
        osiInterfaceInfo *node = calloc(1, sizeof(*node));
        if(!node)
            goto cleanup;

        switch(cur->ifa_addr->sa_family)
        {
        case AF_INET:
        /*case AF_INET6:*/
            break;
        default:
            continue; /* ignore unknown address types */
        }

        assert(cur->ifa_addr->sa_family==cur->ifa_netmask->sa_family);

        memcpy(&node->address.ia, cur->ifa_addr, sizeof(node->address.ia));
        memcpy(&node->netmask.ia, cur->ifa_netmask, sizeof(node->address.ia));

        flags = cur->ifa_flags;

        if(flags&IFF_UP) node->up = 1;
        if(flags&IFF_BROADCAST) node->broadcast = 1;
        if(flags&IFF_MULTICAST) node->multicast = 1;
        if(flags&IFF_LOOPBACK) node->loopback = 1;
        if(flags&IFF_POINTOPOINT) node->point2point = 1;

        if(node->broadcast && node->point2point) {
            errlogPrintf("Interface %s claims both broadcast and point to point,"
                         " which should not be possible.  Assuming broadcast only.",
                         cur->ifa_name);
            node->point2point = 0;
        }

        if(node->broadcast)
            memcpy(&node->endpoint.ia, cur->ifa_broadaddr, sizeof(node->address.ia));
        else if(node->point2point)
            memcpy(&node->endpoint.ia, cur->ifa_dstaddr, sizeof(node->address.ia));

        ellAdd(pList, &node->node);
    }

    ret = 0;
cleanup:
    if(ret)
        ellFree(pList);
    if(addrs) freeifaddrs(addrs);
    return ret;
}

#else /* USE_IFADDRS */

epicsShareFunc int osiGetInterfaceInfo(ELLLIST *pList)
{
    SOCKET sock;
    struct ifconf conf;
    struct ifreq *alloc = NULL, *cur, *next, *end;
    int ret = -1;

    ellFree(pList);

    sock = epicsSocketCreate(AF_INET, SOCK_DGRAM, 0);
    if(sock==INVALID_SOCKET)
        return ret;

    alloc = malloc(maxreq);
    if(!alloc)
        goto cleanup;

    conf.ifc_len = maxreq;
    conf.ifc_req = alloc;
    if(socket_ioctl ( sock, SIOCGIFCONF, &conf )<0)
        goto cleanup;

    /* Some OSs allow struct ifreq to be variable length to support
     * socket addresses with different length.
     * So we must allow that cur may not simply be an array
     * of struct ifreq
     */
    assert(conf.ifc_req==alloc);
    end = (struct ifreq*)(conf.ifc_len+(char*)alloc);

    for(cur=alloc, next=ifreqNext(cur); cur<end; cur=next, next=ifreqNext(next))
    {
        unsigned int flags;
        osiInterfaceInfo *node = calloc(1, sizeof(*node));
        if(!node)
            goto cleanup;

        /* This ifreq node now has the interface name set, but we don't know the
         * state of the union.  So we must use individual ioctl calls to get this information
         */

        if(socket_ioctl(sock, SIOCGIFADDR, cur)<0)
            goto cleanup;

        switch(cur->ifr_addr.sa_family)
        {
        case AF_INET:
        /*case AF_INET6:*/
            break;
        default:
            continue; /* ignore unknown address types */
        }

        memcpy(&node->address.ia, &cur->ifr_addr, sizeof(node->address.ia));

        if(socket_ioctl(sock, SIOCGIFNETMASK, cur)<0)
            goto cleanup;

        assert(cur->ifr_addr.sa_family==node->address.sa.sa_family);

        memcpy(&node->netmask.ia, &cur->ifr_addr, sizeof(node->netmask.ia));

        if(socket_ioctl(sock, SIOCGIFFLAGS, cur)<0)
            goto cleanup;

        flags = cur->ifr_flags;

        if(flags&IFF_UP) node->up = 1;
        if(flags&IFF_BROADCAST) node->broadcast = 1;
        if(flags&IFF_MULTICAST) node->multicast = 1;
        if(flags&IFF_LOOPBACK) node->loopback = 1;
        if(flags&IFF_POINTOPOINT) node->point2point = 1;

        if(node->broadcast && node->point2point) {
            errlogPrintf("Interface %s claims both broadcast and point to point,"
                         " which should not be possible.  Assuming broadcast only.",
                         cur->ifr_name);
            node->point2point = 0;
        }

        if(node->broadcast) {
            if(socket_ioctl(sock, SIOCGIFBRDADDR, cur)<0)
                goto cleanup;
        } else if(node->point2point) {
            if(socket_ioctl(sock, SIOCGIFDSTADDR, cur)<0)
                goto cleanup;
        }

        if(node->broadcast || node->point2point) {
            assert(cur->ifr_addr.sa_family==node->address.sa.sa_family);

            memcpy(&node->endpoint.ia, &cur->ifr_addr, sizeof(node->endpoint.ia));
        }

        ellAdd(pList, &node->node);
    }

    ret = 0;
cleanup:
    if(ret)
        ellFree(pList);
    free(alloc);
    epicsSocketDestroy(sock);
    return ret;
}

#endif /* USE_IFADDRS */
