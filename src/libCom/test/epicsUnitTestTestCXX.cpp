/*************************************************************************\
* Copyright (c) 2017 Michael Davidsaver
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include "epicsUnitTest.h"
#include "testMain.h"

namespace {

struct TestClass {
    TestClass() { testPass("TestClass ctor"); }
    ~TestClass() { testPass("TestClass dtor"); }
    void testOne() { testPass("testOne"); }
    void testTwo() { testPass("testTwo"); }
};

} // namespace

MAIN(epicsUnitTestTestCXX)
{
    testPlan(10);
    try {

        TestCase(true)<<"This is true";
        TestCase(1)<<"So is this";

        testEq(1+1, 2);

        {
            std::string A("hello"), B("hello");
            testEq(A, B);
        }

        testMethod(TestClass, testOne);
        testMethod(TestClass, testTwo);

    }catch(std::exception& e){
        testFail("unexpected exception: %s", e.what());
    }
    return testDone();
}
