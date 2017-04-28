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
#include "epicsThread.h"

#ifdef DEBUG
#   define ifDepenDebugPrintf(argsInParen) printf argsInParen
#else
#   define ifDepenDebugPrintf(argsInParen)
#endif

#ifdef USE_IFADDRS

epicsShareFunc int osiGetInterfaceInfo(ELLLIST *pList, unsigned flags)
{
    int ret = -1;
    struct ifaddrs *addrs = NULL, *cur;

    ellFree(pList);

    if(getifaddrs(&addrs))
        goto cleanup;

    for(cur=addrs; cur; cur=cur?cur->ifa_next:NULL)
//    for(cur=addrs; cur; cur=cur->ifa_next)
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
            free(node);
            continue; /* ignore unknown address types */
        }

        //assert(cur->ifa_addr->sa_family==cur->ifa_netmask->sa_family);
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

epicsShareFunc int osiGetInterfaceInfo(ELLLIST *pList, unsigned flags)
{
    SOCKET sock;
    int ret = -1;
    struct if_nameindex* pIndex = 0;
    struct if_nameindex* pIndex2 = 0;

    ellFree(pList);

    sock = epicsSocketCreate(AF_INET, SOCK_DGRAM, 0);
    if(sock==INVALID_SOCKET)
        return ret;

    pIndex = pIndex2 = if_nameindex();
    while ((pIndex != NULL) && (pIndex->if_name != NULL))
    {
	struct ifreq req;
        unsigned int flags;
        osiInterfaceInfo *node = calloc(1, sizeof(*node));
        if(!node)
            goto cleanup;
	strncpy(req.ifr_name, pIndex->if_name, IFNAMSIZ);
        if(socket_ioctl(sock, SIOCGIFADDR, &req)<0) {
	  if (errno == EADDRNOTAVAIL) {
	    free(node);
            ++pIndex;
            continue;
          }
          free(node);
          goto cleanup;
        }
	memcpy(&node->address.ia, &req.ifr_addr, sizeof(node->address.ia));

        if(socket_ioctl(sock, SIOCGIFNETMASK, &req)<0) {
            free(node);
            goto cleanup;
        }
        memcpy(&node->netmask.ia, &req.ifr_addr, sizeof(node->netmask.ia));

        if(socket_ioctl(sock, SIOCGIFFLAGS, &req)<0) {
            free(node);
            goto cleanup;
        }
        flags = req.ifr_flags;
        if(flags&IFF_UP) node->up = 1;
        if(flags&IFF_BROADCAST) node->broadcast = 1;
        if(flags&IFF_MULTICAST) node->multicast = 1;
        if(flags&IFF_LOOPBACK) node->loopback = 1;
        if(flags&IFF_POINTOPOINT) node->point2point = 1;

        if(node->broadcast && node->point2point) {
            errlogPrintf("Interface %s claims both broadcast and point to point,"
                         " which should not be possible.  Assuming broadcast only.",
                         req.ifr_name);
            node->point2point = 0;
        }
        if(node->broadcast) {
            if(socket_ioctl(sock, SIOCGIFBRDADDR, &req)<0) {
                free(node);
                goto cleanup;
            }
        } else if(node->point2point) {
            if(socket_ioctl(sock, SIOCGIFDSTADDR, &req)<0) {
                free(node);
                goto cleanup;
            }
        }

        if(node->broadcast || node->point2point) {
            assert(req.ifr_addr.sa_family==node->address.sa.sa_family);

            memcpy(&node->endpoint.ia, &req.ifr_addr, sizeof(node->endpoint.ia));
        }

        ellAdd(pList, &node->node);
        ++pIndex;
    }

    ret = 0;
cleanup:
    if(ret)
        ellFree(pList);
    epicsSocketDestroy(sock);
    return ret;
}
#endif /* USE_IFADRS */
