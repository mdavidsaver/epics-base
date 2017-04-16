/*************************************************************************\
* Copyright (c) 2008 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

/*
 * Author: Andrew Johnson
 */

#ifndef INC_epicsUnitTest_H
#define INC_epicsUnitTest_H

#include <stdarg.h>

#ifdef __cplusplus
#include <sstream>
#endif

#include "compilerDependencies.h"
#include "shareLib.h"

#ifdef __cplusplus
extern "C" {
#endif

epicsShareFunc void testPlan(int tests);
epicsShareFunc int  testOkV(int pass, const char *fmt, va_list pvar);
epicsShareFunc int  testOk(int pass, const char *fmt, ...)
						EPICS_PRINTF_STYLE(2, 3);
epicsShareFunc void testPass(const char *fmt, ...)
						EPICS_PRINTF_STYLE(1, 2);
epicsShareFunc void testFail(const char *fmt, ...)
						EPICS_PRINTF_STYLE(1, 2);
epicsShareFunc void testSkip(int skip, const char *why);
epicsShareFunc void testTodoBegin(const char *why);
epicsShareFunc void testTodoEnd(void);
epicsShareFunc int  testDiag(const char *fmt, ...)
						EPICS_PRINTF_STYLE(1, 2);
epicsShareFunc void testAbort(const char *fmt, ...)
						EPICS_PRINTF_STYLE(1, 2);
epicsShareFunc int  testDone(void);

#define testOk1(cond) testOk(cond, "%s", #cond)


typedef int (*TESTFUNC)(void);
epicsShareFunc void testHarness(void);
epicsShareFunc void testHarnessExit(void *dummy);
epicsShareFunc void runTestFunc(const char *name, TESTFUNC func);

#define runTest(func) runTestFunc(#func, func)
#define testHarnessDone() testHarnessExit(0)

#ifdef __cplusplus
}

/** stream based equivalent to testOk()
 *
 * @code
   int a, b;
   ...
   testOk(a==b, "expect %d == %d", a, b);
   // becomes
   TestCase(a==b)<<"expect "<<a<<" = "<<b;
 * @endcode
 */
class TestCase {
    bool ok;
    std::ostringstream strm;
public:
    TestCase(bool ok) : ok(ok) {}
    ~TestCase() {
        testOk(ok, "%s", strm.str().c_str());
    }
    template<typename T>
    TestCase& operator<<(const T& t) {
        strm<<t;
        return *this;
    }
};

/** Quick equality test
 *
 *@code
 std::string a, b;
 testEq(a, b);
 *@endcode
 */
#define testEq(LHS, RHS) TestCase((LHS)==(RHS))<<#LHS<<" ("<<(LHS)<<") == "<<#RHS<<" ("<<(RHS)<<") @"<<__FILE__<<":"<<__LINE__

template<class C, void (C::*M)()>
void testMethod_(const char *kname, const char *mname)
{
    try {
        testDiag("------- %s::%s --------", kname, mname);
        C inst;
        (inst.*M)();
    } catch(std::exception& e) {
#ifdef PRINT_EXCEPTION
        PRINT_EXCEPTION(e);
#endif
        testAbort("unexpected exception: %s", e.what());
    }
}

/** Construct an instance and run one method
 *
 * Helper when many tests have common setup/teardown
 * to be run around each test
 *@code
 struct MyTest {
   MyTest() { ... } // setup
   ~MyTest() { ... } // teardown
   void testOne() { ... }
   void testTwo() { ... }
 };
 MAIN(mytest) {
   ...
   testMethod(MyTest, testOne)
   testMethod(MyTest, testTwo)
   ...
 }
 *@endcode
 */
#define testMethod(klass, method) testMethod_<klass, &klass::method>(#klass, #method)

#endif /* __cplusplus */

#endif /* INC_epicsUnitTest_H */
