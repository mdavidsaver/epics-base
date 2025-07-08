#ifndef DBCOMMONPVT_H
#define DBCOMMONPVT_H

#include <compilerDependencies.h>
#include <dbDefs.h>
#include <ellLib.h>
#include "dbBase.h"
#include "dbCommon.h"

struct epicsThreadOSD;

/* Location where a field was initialized from a DB file
 */
typedef struct {
    ELLNODE node;
    const dbFldDes *flddes;
    const char *filename;
    unsigned int line_num;
} dbFldLocation;

/** Base internal additional information for every record
 */
typedef struct dbCommonPvt {
    struct dbRecordNode *recnode;

    /* Thread which is currently processing this record */
    struct epicsThreadOSD* procThread;

    ELLLIST fldLocations; /* dbFldLocation::node */

    /* actually followed by:
     * struct dbCommon common;
     */
} dbCommonPvt;

static EPICS_ALWAYS_INLINE
dbCommonPvt* dbRec2Pvt(struct dbCommon *prec)
{
    return (dbCommonPvt*)((char*)prec - sizeof(dbCommonPvt));
}

static EPICS_ALWAYS_INLINE
dbCommon* dbPvt2Rec(struct dbCommonPvt *pvt)
{
    return (dbCommon*)&pvt[1];
}

#endif // DBCOMMONPVT_H
