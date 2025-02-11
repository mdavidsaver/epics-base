/*************************************************************************\
* Copyright (c) 2025 Michael Davidsaver
* SPDX-License-Identifier: EPICS
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <epicsMath.h>
#include <freeList.h>
#include <dbConvertFast.h>
#include <chfPlugin.h>
#include <recGbl.h>
#include <epicsExit.h>
#include <dbAccess.h>
#include <epicsExport.h>

typedef struct myStruct {
    int    to;
} myStruct;

#define CAST_SINT 1000
#define CAST_UINT 1000

static const
chfPluginEnumType toEnum[] = {
    /* direct convert to specific */
#define CASE(TYPE) { #TYPE , DBR_ ## TYPE }
    CASE(CHAR),
    CASE(UCHAR),
    CASE(SHORT),
    CASE(USHORT),
    CASE(LONG),
    CASE(ULONG),
    CASE(INT64),
    CASE(FLOAT),
    CASE(DOUBLE),
    //CASE(STRING),
#undef CASE
    /* integer cast with sign change */
    {"SINT", CAST_SINT},
    {"UINT", CAST_UINT},
    {NULL,0}
};

static const
chfPluginArgDef opts[] = {
    chfEnum      (myStruct, to, "to", 0, 1, toEnum),
    chfPluginArgEnd
};

static void * allocPvt(void)
{
    myStruct *my = malloc(sizeof(*my));
    if(my) {
        my->to = -1;
    }
    return my;
}

static void freePvt(void *pvt)
{
    free(pvt);
}

static int parse_ok(void *pvt)
{
    myStruct *my = (myStruct*) pvt;
    (void)my;
    return 0;
}

static db_field_log* filter(void* pvt, dbChannel *chan, db_field_log *pfl) {
    const myStruct *my = (myStruct*) pvt;

    /*
     * Only scalar values supported - strings, arrays, and conversion errors
     * are just passed on
     */
    if (pfl->type == dbfl_type_val
            && pfl->no_elements==1
            && dbChannelFieldSize(chan) <= sizeof(union native_value)) {
        short target = pfl->field_type;

        /* casting rules... */
        if(VALID_DB_REQ(my->to)) {
            target = my->to;

        } else if(my->to==CAST_SINT) {
            switch(target) {
#define CASE(STYPE) case DBR_U ## STYPE: target = DBR_ ## STYPE ; break
            CASE(CHAR);
            CASE(SHORT);
            CASE(LONG);
            CASE(INT64);
#undef CASE
            }

        } else if(my->to==CAST_UINT) {
            switch(target) {
#define CASE(STYPE) case DBR_ ## STYPE: target = DBR_U ## STYPE ; break
            CASE(CHAR);
            CASE(SHORT);
            CASE(LONG);
            CASE(INT64);
#undef CASE
            }
        }
        /* else, no cast */

        if(pfl->field_type!=target && dbValueSize(target)<=sizeof(union native_value)) {
            long status;
            DBADDR localAddr;
            union native_value scratch;
            STATIC_ASSERT(sizeof(scratch)==sizeof(pfl->u.v.field));

            localAddr = chan->addr; /* Structure copy */
            localAddr.field_type = pfl->field_type;
            localAddr.field_size = pfl->field_size;
            localAddr.no_elements = pfl->no_elements;
            localAddr.pfield = (char *) &pfl->u.v.field;

            status = dbFastGetConvertRoutine[pfl->field_type][my->to]
                    (localAddr.pfield, (void*) &scratch, &localAddr);

            if(!status) { /* success */
                /* morph this db_field_log */
                if(pfl->dtor) {
                    pfl->dtor(pfl); /* probably not relevant for scalar, but still */
                    pfl->dtor = NULL;
                }

                pfl->field_type = target;
                pfl->field_size = dbValueSize(target);
                memcpy(&pfl->u.v.field, &scratch, pfl->field_size);
            }
        }
    }
    return pfl;
}

static void channelRegisterPost(dbChannel *chan, void *pvt,
                               chPostEventFunc **cb_out, void **arg_out, db_field_log *probe)
{
    *cb_out = filter;
    *arg_out = pvt;
    probe = filter(pvt, chan, probe); /* safe as probe FL is will not be free'd */
}

static void channel_report(dbChannel *chan, void *pvt, int level, const unsigned short indent)
{
    const myStruct *my = (myStruct*) pvt;
    printf("%*sCast (cast): to=%s\n", indent, "",
           chfPluginEnumString(toEnum, my->to, "n/a"));
}

static const chfPluginIf pif = {
    allocPvt,
    freePvt,

    NULL, /* parse_error, */
    parse_ok,

    NULL, /* channel_open, */
    NULL, /* channelRegisterPre, */
    channelRegisterPost,
    channel_report,
    NULL /* channel_close */
};

static void castInitialize(void)
{
    chfPluginRegister("cast", &pif, opts);
}

epicsExportRegistrar(castInitialize);
