##########################################################################
# Copyright (c) 2011 UChicago Argonne LLC, as Operator of Argonne
#     National Laboratory.
# EPICS BASE is distributed subject to a Software License Agreement found
# in the file LICENSE that is included with this distribution.
##########################################################################

# This is a Makefile fragment, see src/ioc/Makefile.

SRC_DIRS += $(STDDIR)/softIoc

PROD_IOC_DEFAULT = softIoc
PROD_IOC_iOS = -nil-

DBD += base.dbd
DBD += asSub.dbd
DBD += softIoc.dbd

softIoc_DBD += base.dbd
softIoc_DBD += dlload.dbd
softIoc_DBD += system.dbd

softIoc_SRCS += softIoc_registerRecordDeviceDriver.cpp
softIoc_SRCS_DEFAULT += softMain.cpp
softIoc_SRCS_vxWorks = -nil-

softMain_CPPFLAGS += -I$(TOP)/modules/libcom/src/iocsh

softIoc_LIBS = $(EPICS_BASE_IOC_LIBS)

DB += softIocExit.db

CLEANS += epicsInstallDir.h

