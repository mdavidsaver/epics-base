#*************************************************************************
# Copyright (c) 2011 UChicago Argonne LLC, as Operator of Argonne
#     National Laboratory.
# Copyright (c) 2002 The Regents of the University of California, as
#     Operator of Los Alamos National Laboratory.
# EPICS BASE is distributed subject to a Software License Agreement found
# in file LICENSE that is included with this distribution. 
#*************************************************************************

# This is a Makefile fragment, see src/libCom/Makefile.

SRC_DIRS += $(LIBCOM)/flex

parse_YACCOPT = -l -d

# No longer needs to be installed,
# but do it anyway to avoid breaking
# 3rd party builds which copied
# old rules
INC += flex.skel.static

# main.c is included in parse.c
e_flex_SRCS += buf.c
e_flex_SRCS += ccl.c
e_flex_SRCS += dfa.c
e_flex_SRCS += ecs.c
e_flex_SRCS += gen.c
e_flex_SRCS += misc.c
e_flex_SRCS += nfa.c
e_flex_SRCS += sym.c
e_flex_SRCS += tblcmp.c
e_flex_SRCS += options.c
e_flex_SRCS += parse.c
e_flex_SRCS += scanopt.c
e_flex_SRCS += skel.c
e_flex_SRCS += tables_shared.c
e_flex_SRCS += tables.c

PROD_HOST += e_flex

CLEANS += parse.c parse.h
