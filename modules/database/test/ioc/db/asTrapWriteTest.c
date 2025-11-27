/*************************************************************************\
* Copyright (c) 2025 Michael Davidsaver
* SPDX-License-Identifier: EPICS
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <string.h>

#include <asLib.h>
#include <asDbLib.h>
#include <dbDefs.h>
#include <errlog.h>
#include <ellLib.h>
#include <dbAccess.h>
#include <dbChannel.h>
#include <epicsMutex.h>
#include <cantProceed.h>
#include <asTrapWrite.h>
#include <dbUnitTest.h>
#include <testMain.h>

#include "db_access_routines.h"

// copied from db_access.c
#define oldDBF_LONG        5

static
void testListener(asTrapWriteMessage *pmessage,int after)
{
    testOk(strcmp(pmessage->userid, "myself")==0
           && strcmp(pmessage->hostid, "thehost")==0
           && ((!after && !pmessage->userPvt) || (after && pmessage->userPvt==&asActive)),
           "testListener(%s,user:\"%s\",host:\"%s\",pv:\"%s\",dbr:%d,no_elem:%d,data:%p",
           after ? "after" : "before",
           pmessage->userid,
           pmessage->hostid,
           pmessage->serverSpecific->name,
           pmessage->dbrType,
           pmessage->no_elements,
           pmessage->data);

    if(!after) {
        pmessage->userPvt = &asActive;
    }

    if(!after)
        testdbGetFieldEqual("target", DBF_LONG, 42);
    else
        testdbGetFieldEqual("target", DBF_LONG, 43);

    //pmessage->data
    // RSRV sets this on !after with the value to be Put
    // QSRV1/2 passes NULL
    // caPutLog ignores
}

static
void testRemotePut(const char *pv, int dbf, int count, void* buf)
{
    char host[] = "theHost"; // asAddClient() overwrites with lower
    dbChannel *target = dbChannelCreate(pv);
    if(!target || dbChannelOpen(target))
        testAbort("no PV: %s", pv);

    ASCLIENTPVT pvt = NULL;
    testOk1(!asAddClient(&pvt, asDbGetMemberPvt(target), asDbGetAsl(target), "myself", host));

    // RSRV does not dbScanLock()
    void *wpvt = asTrapWriteWithData(pvt, "myself", host, target, dbf, count, buf);

    testOk1(!dbChannel_put(target, dbf, buf, count));

    asTrapWriteAfter(wpvt);

    testOk1(!asRemoveClient(&pvt));
    dbChannelDelete(target);
}

void dbTestIoc_registerRecordDeviceDriver(struct dbBase *);

MAIN(asTrapWriteTest)
{
    testPlan(9);
    testdbPrepare();

    testdbReadDatabase("dbTestIoc.dbd", NULL, NULL);
    dbTestIoc_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("asTrapWriteTest.db", NULL, NULL);

    testOk1(!asSetFilename("../asTrapWriteTest.acf"));

    asTrapWriteId trapid = asTrapWriteRegisterListener(&testListener);

    eltc(0);
    testIocInitOk();
    eltc(1);
    testOk1(asActive);

    {
        epicsInt32 val = 43;
        testRemotePut("target", oldDBF_LONG, 1, &val);
    }

    testIocShutdownOk();

    asTrapWriteUnregisterListener(trapid);

    testdbCleanup();

    return testDone();
}
