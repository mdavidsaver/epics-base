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

#include "compilerDependencies.h"
#include "libComAPI.h"

#ifdef __cplusplus
extern "C" {
#endif

LIBCOM_API void testPlan(int tests);
LIBCOM_API int  testOkV(int pass, const char *fmt, va_list pvar);
LIBCOM_API int  testOk(int pass, const char *fmt, ...)
						EPICS_PRINTF_STYLE(2, 3);
LIBCOM_API void testPass(const char *fmt, ...)
						EPICS_PRINTF_STYLE(1, 2);
LIBCOM_API void testFail(const char *fmt, ...)
						EPICS_PRINTF_STYLE(1, 2);
LIBCOM_API void testSkip(int skip, const char *why);
LIBCOM_API void testTodoBegin(const char *why);
LIBCOM_API void testTodoEnd(void);
LIBCOM_API int  testDiag(const char *fmt, ...)
						EPICS_PRINTF_STYLE(1, 2);
LIBCOM_API void testAbort(const char *fmt, ...)
						EPICS_PRINTF_STYLE(1, 2);
LIBCOM_API int  testDone(void);

#define testOk1(cond) testOk(cond, "%s", #cond)


typedef int (*TESTFUNC)(void);
LIBCOM_API void testHarness(void);
LIBCOM_API void testHarnessExit(void *dummy);
LIBCOM_API void runTestFunc(const char *name, TESTFUNC func);

#define runTest(func) runTestFunc(#func, func)
#define testHarnessDone() testHarnessExit(0)

#ifdef __cplusplus
}
#endif

#endif /* INC_epicsUnitTest_H */
