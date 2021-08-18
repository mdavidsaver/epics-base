/*************************************************************************\
* Copyright (c) 2021 Lucas Russo
* SPDX-License-Identifier: EPICS
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#ifndef INC_dbAccessNoThrow_H
#define INC_dbAccessNoThrow_H

#include <dbCoreAPI.h>

#ifdef __cplusplus
extern "C" {
#endif

DBCORE_API long dbRecSupGetVFieldNoThrow(rset *prset,
        struct dbAddr *paddr, struct VField *vfield);
DBCORE_API long dbRecSupPutVFieldNoThrow(rset *prset,
        struct dbAddr *paddr, const struct VField *vfield);

#ifdef __cplusplus
}
#endif

#endif /* INC_dbAccessNoThrow_H */
