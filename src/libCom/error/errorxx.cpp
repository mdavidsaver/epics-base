/*************************************************************************\
* Copyright (c) 2017 Michael Davidsaver
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include "errMdef.h"

namespace epics {

const char* Error::what() const EPICS_NOEXCEPT
{
    return errSymMsg(sts);
}

} // namespace epics
