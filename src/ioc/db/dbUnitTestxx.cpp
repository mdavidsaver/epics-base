/*************************************************************************\
* Copyright (c) 2017 Michael Davidsaver
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
 \*************************************************************************/

#include "errlog.h"

#define epicsExportSharedSymbols
#include "dbUnitTest.h"


TestIOC::TestIOC()
    :hasInit(false)
{
    testdbPrepare();
}

TestIOC::~TestIOC() {
    this->shutdown();
    testdbCleanup();
}

void TestIOC::readDatabase(const char* file,
                           const char* path,
                           const char* substitutions)
{
    testdbReadDatabase(file, path, substitutions);
}

void TestIOC::init() {
    if(!hasInit) {
        eltc(0);
        testIocInitOk();
        eltc(1);
        hasInit = true;
    }
}

void TestIOC::shutdown() {
    if(hasInit) {
        testIocShutdownOk();
        hasInit = false;
    }
}
