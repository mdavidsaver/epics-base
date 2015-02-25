#*************************************************************************
# Copyright (c) 2006 The University of Chicago, as Operator of Argonne
#     National Laboratory.
# Copyright (c) 2002 The Regents of the University of California, as
#     Operator of Los Alamos National Laboratory.
# EPICS BASE is distributed subject to a Software License Agreement found
# in file LICENSE that is included with this distribution.
#*************************************************************************
TOP=../../..

include $(TOP)/configure/CONFIG

PROD_LIBS += Com dbdast

TESTPROD_HOST += streamTest
streamTest_SRCS += streamTest.cpp
#testHarness_SRCS += streamTest.cpp
TESTS += streamTest

TESTPROD_HOST += lexTest
lexTest_SRCS += lexTest.cpp
#testHarness_SRCS += lexTest.cpp
TESTS += lexTest

# The testHarness runs all the test programs in a known working order.
#testHarness_SRCS += epicsRunLibComTests.c

#dbdastTestHarness_SRCS += $(testHarness_SRCS)
#dbdastTestHarness_SRCS_RTEMS += rtemsTestHarness.c

#PROD_vxWorks = dbdastTestHarness
#PROD_RTEMS += dbdastTestHarness

#TESTSPEC_vxWorks = dbdastTestHarness.munch; epicsRunLibComTests
#TESTSPEC_RTEMS = dbdastTestHarness.boot; epicsRunLibComTests

TESTSCRIPTS_HOST += $(TESTS:%=%.t)


include $(TOP)/configure/RULES

