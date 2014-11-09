#*************************************************************************
# Copyright (c) 2002 The University of Chicago, as Operator of Argonne
#     National Laboratory.
# Copyright (c) 2002 The Regents of the University of California, as
#     Operator of Los Alamos National Laboratory.
# EPICS BASE is distributed subject to a Software License Agreement found
# in file LICENSE that is included with this distribution. 
#*************************************************************************

# This is a Makefile fragment, see src/libCom/Makefile.

SRC_DIRS += $(LIBCOM)/yacc

# Install both skeleton files.
# yaccpar.skel is the default
INC += btyaccpar.skel
INC += yaccpar.skel

antelope_SRCS += closure.c
antelope_SRCS += error.c
antelope_SRCS += graph.c
antelope_SRCS += lalr.c
antelope_SRCS += lr0.c
antelope_SRCS += main_yacc.c
antelope_SRCS += mkpar.c
antelope_SRCS += mstring.c
antelope_SRCS += output.c
antelope_SRCS += reader.c
antelope_SRCS += skeleton.c
antelope_SRCS += symtab.c
antelope_SRCS += verbose.c
antelope_SRCS += warshall.c

PROD_HOST += antelope

