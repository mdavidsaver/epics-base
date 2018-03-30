/*************************************************************************\
* Copyright (c) 2018 Michael Davidsaver
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <string>

#include <stdlib.h>
#include <string.h>

#define epicsExportSharedSymbols
#include <epicsStdio.h> // redirect stdout/err
#include <epicsStdlib.h>
#include <epicsStdio.h>
#include <dbDefs.h>
#include <yajl_parse.h>
#include <epicsAssert.h>

#include "jtreepvt.h"

namespace epics{namespace jtree{


ParseError::ParseError(size_t pos, size_t line, size_t col, const std::string &msg)
    :std::runtime_error(msg)
    ,pos(pos)
    ,line(line)
    ,col(col)
{}

ParseError::~ParseError() throw() {}

namespace {
struct Error {
    size_t pos;
    std::string msg;

    static void error(void *errpvt, size_t pos, const char *msg) {
        Error *self = (Error*)errpvt;
        try {
            self->pos = pos;
            self->msg = msg;
        }catch(std::exception& e){
            fprintf(stderr, "Error which reporting error in %s:%d : %s : %s\n", __FILE__, __LINE__, msg, e.what());
        }
    }
};
} // namespace

void Tree::parse(const char *inp, size_t inplen, unsigned flags)
{
    Error err;
    Tree temp(ejtParse(inp, inplen, &Error::error, (void*)&err, flags));
    if(!temp) {
        size_t line=0, col=0;
        ejtPos2Line(inp, inplen, err.pos, &line, &col);
        throw ParseError(err.pos, line, col, err.msg);
    }

    swap(temp);
}

}} // namespace epics::jtree
