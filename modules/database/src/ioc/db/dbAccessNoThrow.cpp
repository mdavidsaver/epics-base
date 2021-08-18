/*************************************************************************\
* Copyright (c) 2021 Lucas Russo
* SPDX-License-Identifier: EPICS
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <stdexcept>

#include "recSup.h"
#include "dbAccessDefs.h"
#include "errlog.h"
#include "dbAccessNoThrow.h"

/*
 * Maps possible excpetions to DB error codes. Using the
 * "exception dispatcher" pattern.
 */
inline long dbExceptionToLong()
{
    try {
        throw;
    }
    catch (std::exception& e) {
        errlogPrintf("dbExceptionToLong: Unhandled exception %s: %s\n",
                typeid(e).name(), e.what());
        return S_db_errArg;
    }
}

/*
 * Pseudo variadic templates for use without C++11 features
 */

template <typename F, typename T1, typename T2>
long dbRecSupNoThrowWrapper(F f, T1 t1, T2 t2) {
    try {
        return f(t1, t2);
    }
    catch(...){
        return dbExceptionToLong();
    }
}

/*
 * Wraps all RecordSupport functions from throwable functions to no-throwable ones,
 * without using C++11 features
 *
 * For use with C code
 */

long dbRecSupGetVFieldNoThrow(rset *prset, struct dbAddr *paddr, struct VField *vfield)
{
    return dbRecSupNoThrowWrapper(prset->get_vfield, paddr, vfield);
}

long dbRecSupPutVFieldNoThrow(rset *prset, struct dbAddr *paddr, const struct VField *vfield)
{
    return dbRecSupNoThrowWrapper(prset->put_vfield, paddr, vfield);
}
