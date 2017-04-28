/*************************************************************************\
* Copyright (c) 2015 Brookhaven Science Associates as Operator of
*     Brookhaven National Lab.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <stdlib.h>

#include "dbDefs.h"
#include "osiSock.h"
#include "epicsTypes.h"

#include "epicsUnitTest.h"
#include "testMain.h"

static
void testIfInfo(void)
{
    int foundlo = 0, bcastok = 1;
    ELLLIST iflist = ELLLIST_INIT;
    ELLNODE *cur;
    testDiag("Check interface introspection info");

#ifdef USE_IFADDRS
    testDiag("Using getifaddrs() method");
#else
    testDiag("Using OS default method");
#endif

    testOk1(osiGetInterfaceInfo(&iflist, 0)==0);

    testOk(ellCount(&iflist)>0, "interface count %d", ellCount(&iflist));

    for(cur=ellFirst(&iflist); cur; cur=ellNext(cur))
    {
        osiInterfaceInfo *info = CONTAINER(cur, osiInterfaceInfo, node);
        char buf[30];

        if(info->loopback) {
            testOk(info->up, "loopback interface is up");
            foundlo = 1;
        }

        if(info->address.sa.sa_family!=AF_INET)
            continue;

        ipAddrToDottedIP(&info->address.ia, buf, sizeof(buf));
        testDiag("Address: %s", buf);
        ipAddrToDottedIP(&info->netmask.ia, buf, sizeof(buf));
        testDiag("Netmask: %s", buf);
        if(info->broadcast) {
            ipAddrToDottedIP(&info->endpoint.ia, buf, sizeof(buf));
            testDiag("Broadcast: %s", buf);
        } else if(info->point2point) {
            ipAddrToDottedIP(&info->endpoint.ia, buf, sizeof(buf));
            testDiag("Destination: %s", buf);
        }

        testDiag(" Up: %s", info->up?"Up":"Down");
        testDiag(" Loopback: %s", info->loopback?"Yes":"No");
        testDiag(" Broadcast: %s", info->broadcast?"Yes":"No");
        testDiag(" Multicast: %s", info->multicast?"Yes":"No");
        testDiag(" Point2Point: %s", info->point2point?"Yes":"No");

        /* check consistency of address, netmask, and broadcast address */
        if(info->broadcast) {
            epicsUInt32 addr  = ntohl(info->address.ia.sin_addr.s_addr),
                        mask  = ntohl(info->netmask.ia.sin_addr.s_addr),
                        bcast = ntohl(info->endpoint.ia.sin_addr.s_addr),
                        bcast2= (addr&mask) | ~mask;

            if(bcast!=bcast2) {
                struct sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_addr.s_addr = htonl(bcast2);
                addr.sin_port = 0;
                ipAddrToDottedIP(&addr, buf, sizeof(buf));
                testDiag("Warning: expected broadcast address %s", buf);
                bcastok = 0;
            }
        }
    }

    ellFree2(&iflist, (FREEFUNC)osiFreeInterfaceInfo);

    testOk(foundlo, "Found loopback interface");
    testOk(bcastok, "Broadcast addresses consistent");
}

static
void testBroadcast(void)
{
    ELLLIST iflist = ELLLIST_INIT;
    ELLNODE *cur;
    SOCKET sock;
    osiSockAddr match;

    testDiag("Discover broadcast addresses");

    sock = epicsSocketCreate(AF_INET, SOCK_DGRAM, 0);
    if(sock==INVALID_SOCKET)
        testAbort("Failed to allocate socket");

    match.ia.sin_family = AF_INET;
    match.ia.sin_addr.s_addr = htonl(INADDR_ANY);
    match.ia.sin_port = 0;

    osiSockDiscoverBroadcastAddresses(&iflist, sock, &match);

    testOk(ellCount(&iflist)>0, "broadcast count %d", ellCount(&iflist));

    for(cur=ellFirst(&iflist); cur; cur=ellNext(cur))
    {
        osiSockAddrNode *info = CONTAINER(cur, osiSockAddrNode, node);
        char buf[30];

        ipAddrToDottedIP(&info->addr.ia, buf, sizeof(buf));
        testDiag("Broadcast: %s", buf);
    }

    ellFree(&iflist);

    testDiag("\"Discover\" loopback address");

    match.ia.sin_family = AF_INET;
    match.ia.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    match.ia.sin_port = 0;

    osiSockDiscoverBroadcastAddresses(&iflist, sock, &match);

    testOk(ellCount(&iflist)>0, "broadcast count %d", ellCount(&iflist));

    for(cur=ellFirst(&iflist); cur; cur=ellNext(cur))
    {
        osiSockAddrNode *info = CONTAINER(cur, osiSockAddrNode, node);
        char buf[30];

        ipAddrToDottedIP(&info->addr.ia, buf, sizeof(buf));
        testDiag("Broadcast: %s", buf);
    }

    ellFree(&iflist);

    epicsSocketDestroy(sock);
}

static
void testLocal(void)
{
    SOCKET sock;
    osiSockAddr addr;
    char buf[30];

    testDiag("Discover first local address");

    sock = epicsSocketCreate(AF_INET, SOCK_DGRAM, 0);
    if(sock==INVALID_SOCKET)
        testAbort("Failed to allocate socket");

    addr = osiLocalAddr(sock);

    testOk1(addr.sa.sa_family==AF_INET);

    ipAddrToDottedIP(&addr.ia, buf, sizeof(buf));
    testDiag("Address: %s", buf);

    epicsSocketDestroy(sock);
}

static
void testBroadcastMatch(void)
{
    SOCKET sock;
    osiSockAddr match;
    ELLNODE *cur;
    ELLLIST iflist = ELLLIST_INIT;
    char buf[30];

    testDiag("Check osiSockDiscoverBroadcastAddresses() w/ matching");

    sock = epicsSocketCreate(AF_INET, SOCK_DGRAM, 0);
    if(sock==INVALID_SOCKET)
        testAbort("Failed to allocate socket");

    match = osiLocalAddr(sock);

    testOk1(match.sa.sa_family==AF_INET);
    ipAddrToDottedIP(&match.ia, buf, sizeof(buf));
    testDiag("Address: %s", buf);

    osiSockDiscoverBroadcastAddresses(&iflist, sock, &match);

    testOk(ellCount(&iflist)>0, "broadcast count %d", ellCount(&iflist));

    for(cur=ellFirst(&iflist); cur; cur=ellNext(cur))
    {
        osiSockAddrNode *info = CONTAINER(cur, osiSockAddrNode, node);
        char buf[30];

        ipAddrToDottedIP(&info->addr.ia, buf, sizeof(buf));
        testDiag("Broadcast: %s", buf);
    }

    ellFree(&iflist);

    epicsSocketDestroy(sock);
}

MAIN(epicsNetIntfTest)
{
    testPlan(10);
    osiSockAttach();
    testIfInfo();
    testBroadcast();
    testLocal();
    testBroadcastMatch();
    osiSockRelease();
    return testDone();
}
