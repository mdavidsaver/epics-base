/*************************************************************************\
* Copyright (c) 2015 Brookhaven Science Assoc. as operator of Brookhaven
*               National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
 \*************************************************************************/
/*
 * This test will leak memory during tests of parser failure
 */

#include "string.h"

#include "epicsString.h"
#include "dbUnitTest.h"
#include "epicsThread.h"
#include "iocInit.h"
#include "dbBase.h"
#include "link.h"
#include "dbAccess.h"
#include "iocsh.h"
#include "registry.h"
#include "dbStaticLib.h"
#include "dbStaticPvt.h"
#include "osiFileName.h"
#include "dbmf.h"
#include "errlog.h"

#include "xRecord.h"

#include "testMain.h"

#define testIntEqual(A,B) testOk((A)==(B), #A " (%d) == " #B " (%d)", A, B)

void dbTestIoc_registerRecordDeviceDriver(struct dbBase *);

static void testNormal(void)
{
    testDiag("Parse normal syntax");
    testdbPrepare();

    testdbReadDatabase("dbTestIoc.dbd", NULL, NULL);

    dbTestIoc_registerRecordDeviceDriver(pdbbase);

    testdbReadDatabase("dbParseTest1.db", NULL, NULL);

    testIntEqual(12, ellCount(&pdbbase->menuList));
    testIntEqual(1, ellCount(&pdbbase->recordTypeList));
    testIntEqual(0, ellCount(&pdbbase->drvList));
    testIntEqual(0, ellCount(&pdbbase->registrarList));
    testIntEqual(0, ellCount(&pdbbase->functionList));
    testIntEqual(0, ellCount(&pdbbase->variableList));
    testIntEqual(1, ellCount(&pdbbase->bptList));
    testIntEqual(0, ellCount(&pdbbase->filterList));
    testIntEqual(0, ellCount(&pdbbase->blockIgnoreList));

    testdbCleanup();
}

static void testExtendOk(void)
{
    testDiag("Parse extended syntax");

    testdbPrepare();

    testdbReadDatabase("dbTestIoc.dbd", NULL, NULL);

    dbTestIoc_registerRecordDeviceDriver(pdbbase);

    testdbReadDatabase("dbParseTest2.db", NULL, NULL);

    testIntEqual(13, ellCount(&pdbbase->menuList));
    testIntEqual(1, ellCount(&pdbbase->recordTypeList));
    testIntEqual(0, ellCount(&pdbbase->drvList));
    testIntEqual(0, ellCount(&pdbbase->registrarList));
    testIntEqual(0, ellCount(&pdbbase->functionList));
    testIntEqual(0, ellCount(&pdbbase->variableList));
    testIntEqual(0, ellCount(&pdbbase->bptList));
    testIntEqual(0, ellCount(&pdbbase->filterList));
    testIntEqual(2, ellCount(&pdbbase->blockIgnoreList));

    testdbCleanup();
}

static void testParseFail(const char *name, long expect)
{
    long ret;

    testDiag("Parse failure %s", name);
    testdbPrepare();

    testdbReadDatabase("dbTestIoc.dbd", NULL, NULL);

    dbTestIoc_registerRecordDeviceDriver(pdbbase);

    eltc(0);
    ret = dbReadDatabase(&pdbbase, name, "." OSI_PATH_LIST_SEPARATOR ".." OSI_PATH_LIST_SEPARATOR
                         "../O.Common" OSI_PATH_LIST_SEPARATOR "O.Common", NULL);
    eltc(1);


    if(ret==expect)
        testPass("%s fails as expected", name);
    else
        testFail("%s did not fail as expected %ld!=%ld", name,ret,expect);

    testdbCleanup();
}

static int testcnt;

static const iocshArg Arg0 = { "num", iocshArgInt};
static const iocshArg * const Args[] = {&Arg0};
static const iocshFuncDef FuncDef = {"testCmd",1,Args};
static void CallFunc(const iocshArgBuf *args)
{
    testcnt += args[0].ival;
}

static void testIocsh(void)
{
    testDiag("Call iocsh from db");
    iocshRegister(&FuncDef, CallFunc);

    testdbPrepare();

    testdbReadDatabase("dbTestIoc.dbd", NULL, NULL);

    dbTestIoc_registerRecordDeviceDriver(pdbbase);

    testIntEqual(testcnt, 0);

    eltc(0);
    testdbReadDatabase("dbIocsh.db", NULL, NULL);
    eltc(1);

    testIntEqual(testcnt, 3);

    testdbCleanup();
}


MAIN(dbParseTest)
{
    testPlan(0);
    testNormal();
    testExtendOk();
    /* gives "Unknown keyword: "foo"" */
    testParseFail("dbParseTest3.db", -1);
    /* syntax error at or before "field".
     * Leaves parser stack dirty
     */
    testParseFail("dbParseTest4.db", -1);
    testIocsh();
    return testDone();
}
