#*************************************************************************
# Copyright (c) 2007 UChicago Argonne LLC, as Operator of Argonne
#     National Laboratory.
# Copyright (c) 2002 The Regents of the University of California, as
#     Operator of Los Alamos National Laboratory.
# EPICS BASE is distributed subject to a Software License Agreement found
# in file LICENSE that is included with this distribution. 
#*************************************************************************
TOP=../..

include $(TOP)/configure/CONFIG

LIBRARY_IOC += dbdast
dbdast_LIBS += Com

PROD_LIBS = Com

dbdast_SRCS += cfstream.cpp
dbdast_SRCS += macstream.cpp

INC += cfstream.h
INC += macstream.h

include $(TOP)/configure/RULES
