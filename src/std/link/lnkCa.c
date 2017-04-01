/*************************************************************************\
* Copyright (c) 2017 Michael Davidsaver
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "alarm.h"
#include "dbDefs.h"
#include "errlog.h"
#include "epicsAssert.h"
#include "epicsAtomic.h"
#include "epicsString.h"
#include "epicsTypes.h"
#include "epicsMutex.h"
#include "epicsThread.h"
#include "epicsExit.h"

/* Note: we are including db_access.h here
 * and thus can't have anything to do with
 * dbAccess.h
 */
#include "cadef.h"

#include "db_convert.h"

#include "dbCommon.h"
#include "dbScan.h"
#include "dbLink.h"
#include "recGbl.h"
#include "dbJLink.h"
#include "epicsExport.h"

typedef enum {
    lnkCaParsePV = 0,
    lnkCaParsePROC,
    lnkCaParseMS,
    lnkCaParseDEBUG,
    lnkCaParseQOS,
    lnkCaParseINVALID
} lnkCaParseState;

typedef enum {
    lnkCaQOS_None,
    /* accept put request while disconnected,
     * issue put upon connection.
     * dis-connect before ack. not retried
     */
    lnkCaQOS_defer,
    /* accept put request while disconnected,
     * issue put upon connection or data update
     * when not equal.
     */
    lnkCaQOS_sync,
} lnkCaQOS;

typedef struct {
    jlink link;
    int refcount;

    lnkCaParseState pstate;

    epicsMutexId lock;

    struct link *plink;

    /* target PV name */
    char *target;

    chid channel;
    evid sub_value;
    evid sub_meta;

    chtype native_type, /* native channel type (eg. DBR_DOUBLE) */
           value_type,  /* subscribed type for values (eg. DBR_TIME_DOUBLE) */
           put_type;    /* type of last value to put */

    /* storage for value subscription */
    epicsUInt16 status, sevr;
    epicsTimeStamp time;
    void *getval;
    size_t getvalcount;

    /* last value put */
    void *putval;
    size_t putvalcount, putvalcountalloc;

    /* storage for meta-data subscription.
     * lset API uses double, so we always request this :)
     */
    struct dbr_ctrl_double meta;

    /* we re-use some of the pvlOpt* flags */
    unsigned flags;

    unsigned debug:2;
    unsigned datavalid:1;
    unsigned metavalid:1;
    unsigned putretry:1;
    unsigned scanqueued:2; /* counts 0, 1, 2 */
} ca_link;

static
void lnkca_free(jlink *lnk);

static
epicsThreadOnceId lnkca_once = EPICS_THREAD_ONCE_INIT;

static
struct ca_client_context *lnkca_context;

/* internal */

static
void linkca_scan_complete(void *raw, dbCommon *prec)
{
    ca_link *link = raw;

    epicsMutexMustLock(link->lock);
    if(!link->plink) {
        /* IOC shutdown or link re-targeted.  Do nothing. */
    } else if(--link->scanqueued) {
        if(scanOnceCallback(link->plink->precord, &linkca_scan_complete, link)) {
            errlogPrintf("%s: ca_link failed to re-queue scan\n", prec->name);
            link->scanqueued = 0;
        } else {
            epicsAtomicIncrIntT(&link->refcount);
            if(link->debug)
                errlogPrintf("%s -> %s: ca_link re-queued scan\n",
                             link->plink->precord->name, link->target);
        }
    }

    epicsMutexUnlock(link->lock);

    lnkca_free(&link->link);
}

/* link->lock must be locked */
static
void linkca_scan(ca_link *link)
{
    assert(link->plink); /* not remove'd() */

    if(!((link->flags & pvlOptCP) ||
            ((link->flags & pvlOptCPP) && link->plink->precord->scan==0)))
        return;

    switch(link->scanqueued) {
    case 1:
        link->scanqueued++;
    case 2:
        return;
    case 0:
        break;
    }

    if(scanOnceCallback(link->plink->precord, &linkca_scan_complete, link)) {
        errlogPrintf("%s: ca_link failed to queue scan\n", link->plink->precord->name);
    } else {
        epicsAtomicIncrIntT(&link->refcount);
        link->scanqueued = 1;
        if(link->debug)
            errlogPrintf("%s -> %s: ca_link queued scan\n",
                         link->plink->precord->name, link->target);
    }
}

/* CA callbacks */

static
void linkca_ca_error(struct exception_handler_args args)
{
    const char *context = (args.ctx ? args.ctx : "unknown");

    errlogPrintf("ca_link Link Exception: \"%s\", context \"%s\"\n",
        ca_message(args.stat), context);
    if (args.chid) {
        errlogPrintf(
            "ca_link Link Exception: channel \"%s\"\n",
            ca_name(args.chid));
        if (ca_state(args.chid) == cs_conn) {
            errlogPrintf(
                "ca_link Link Exception:  native  T=%s, request T=%s,"
                " native N=%ld, request N=%ld, "
                " access rights {%s%s}\n",
                dbr_type_to_text(ca_field_type(args.chid)),
                dbr_type_to_text(args.type),
                ca_element_count(args.chid),
                args.count,
                ca_read_access(args.chid) ? "R" : "",
                ca_write_access(args.chid) ? "W" : "");
        }
    }
}

static
void linkca_new_data(struct event_handler_args args)
{
    ca_link *link = ca_puser(args.chid);

    epicsMutexMustLock(link->lock);
    assert(link->channel==args.chid);

    if(link->debug)
        errlogPrintf("%s -> %s: ca_link event dtype=%u count=%u status=0x%x\n",
                     link->plink->precord->name, link->target, (unsigned)args.type,
                     (unsigned)args.count, (unsigned)args.status);

    if(args.status!=ECA_NORMAL) {
        /* not sure if/when this happens, but it isn't good.
         * flag as invalid
         */

        link->status = LINK_ALARM;
        link->sevr   = INVALID_ALARM;
        epicsTimeGetCurrent(&link->time);

        if(args.type==link->value_type) {
            link->datavalid = 0;
        } else if(args.type==DBR_CTRL_DOUBLE) {
            link->metavalid = 0;
        } else {
            link->datavalid = link->metavalid = 0;
        }

    } else if(args.type==link->value_type) {

        const struct dbr_time_char *sts = args.dbr;
        link->sevr = sts->severity;
        link->status = sts->status;
        link->time = sts->stamp;

        link->getvalcount = args.count;

        memcpy(link->getval,
               dbr_value_ptr(args.dbr, args.type),
               dbr_size_n(link->native_type, args.count));

        link->datavalid = 1;

    } else if(args.type==DBR_CTRL_DOUBLE) {

        memcpy(&link->meta,
               args.dbr,
               sizeof(link->meta));

        link->metavalid = 1;
    }

    linkca_scan(link);

    epicsMutexUnlock(link->lock);
}

static
void lnkca_conn_status(struct connection_handler_args args)
{
    ca_link *link = ca_puser(args.chid);

    epicsMutexMustLock(link->lock);
    assert(link->channel==args.chid);

    if(link->debug)
        errlogPrintf("%s -> %s: ca_link %sconnect\n",
                     link->plink->precord->name, link->target, args.op==CA_OP_CONN_UP ? "" : "dis");

    if(args.op==CA_OP_CONN_UP) {
        /* max element count */
        unsigned long count = ca_element_count(args.chid);

        link->native_type = ca_field_type(args.chid);

        if(link->native_type==DBF_INT) {
            /* force demote enum -> int until we handle meta-data aware converts */
            link->native_type = DBF_STRING;
        } else if(INVALID_DB_FIELD(link->native_type)) {
            /* ask server to convert unknown types to string */
            link->native_type = DBF_STRING;
        }

        link->value_type = dbf_type_to_DBR_TIME(link->native_type);

        free(link->getval);
        link->getval = malloc(dbr_size_n(link->native_type, count));
        if(!link->getval) {
            errlogPrintf("ca_link failed to allocate RX buffer for \"%s\" %u %lu\n",
                         ca_name(args.chid), (unsigned)link->native_type, count);

        } else {
            int ret = ca_add_masked_array_event(link->value_type, 0, link->channel,
                                                linkca_new_data, link,
                                                0.0, 0.0, 0.0,
                                                &link->sub_value, DBE_VALUE|DBE_ALARM);

            if(ret!=ECA_NORMAL) {
                errlogPrintf("ca_link failed to add value subscription to \"%s\"\n",
                             ca_name(args.chid));
                assert(!link->sub_value);
            }

            ret = ca_add_masked_array_event(DBR_CTRL_DOUBLE, 1, link->channel,
                                            linkca_new_data, link,
                                            0.0, 0.0, 0.0,
                                            &link->sub_meta, DBE_PROPERTY);

            if(ret!=ECA_NORMAL) {
                errlogPrintf("ca_link failed to add meta subscription to \"%s\"\n",
                             ca_name(args.chid));
                assert(!link->sub_meta);
            }
        }

    } else if(args.op==CA_OP_CONN_DOWN) {

        if(link->sub_value)
            SEVCHK(ca_clear_subscription(link->sub_value),
                   "remove clear value");
        if(link->sub_meta)
            SEVCHK(ca_clear_subscription(link->sub_meta),
                   "remove clear meta");


        link->status = LINK_ALARM;
        link->sevr   = INVALID_ALARM;
        link->metavalid = 0;

        linkca_scan(link);
    }

    epicsMutexUnlock(link->lock);

    /* needed? */
    SEVCHK(ca_flush_io(),
           "open flush");
}

/* link support calls */

static
void lnkca_open(struct link *plink)
{
    struct ca_client_context *save;
    ca_link *link = CONTAINER(plink->value.json.jlink, ca_link, link);

    if(!link->target) {
        errlogPrintf("ca_link w/o target PV\n");
        return;
    }

    if(link->debug)
        errlogPrintf("%s -> %s: ca_link open\n", plink->precord->name, link->target);

    link->plink = plink;

    save = ca_current_context();
    if(save)
        ca_detach_context();
    SEVCHK(ca_attach_context(lnkca_context),
           "open attach");

    epicsMutexMustLock(link->lock);

    int ret = ca_create_channel(link->target, lnkca_conn_status, link, CA_PRIORITY_DB_LINKS,
                                &link->channel);
    if(ret!=ECA_NORMAL) {
        errlogPrintf("ca_link: bad PV name \"%s\"\n", link->target);
        link->channel = NULL;
    }

    epicsMutexUnlock(link->lock);

    SEVCHK(ca_flush_io(),
           "open flush");

    ca_detach_context();
    if(save)
        ca_attach_context(save);
}

static
void lnkca_remove(struct dbLocker *locker, struct link *plink)
{
    struct ca_client_context *save;
    ca_link *link = CONTAINER(plink->value.json.jlink, ca_link, link);

    save = ca_current_context();
    if(save)
        ca_detach_context();
    SEVCHK(ca_attach_context(lnkca_context),
           "open attach");

    epicsMutexMustLock(link->lock);

    if(link->debug)
        errlogPrintf("%s -> %s: ca_link remove\n",
                     link->plink->precord->name, link->target);

    if(link->sub_value)
        SEVCHK(ca_clear_subscription(link->sub_value),
               "remove clear value");
    if(link->sub_meta)
        SEVCHK(ca_clear_subscription(link->sub_meta),
               "remove clear meta");
    if(link->channel)
        SEVCHK(ca_clear_channel(link->channel),
               "remove clear chan");

    link->sub_value = link->sub_meta = NULL;
    link->channel = NULL;

    link->plink = NULL;

    epicsMutexUnlock(link->lock);

    ca_detach_context();
    if(save)
        ca_attach_context(save);

    lnkca_free(&link->link);
}

static
int lnkca_isconn(const struct link *plink)
{
    ca_link *link = CONTAINER(plink->value.json.jlink, ca_link, link);
    int ret;

    epicsMutexMustLock(link->lock);
    /* channel connected, and first data update received */
    ret = link->sub_value!=NULL && link->datavalid;
    if(link->debug>1)
        errlogPrintf("%s -> %s: ca_link isconn=%d\n", link->plink->precord->name, link->target, ret);
    epicsMutexUnlock(link->lock);
    return ret;
}

static
int lnkca_dtype(const struct link *plink)
{
    ca_link *link = CONTAINER(plink->value.json.jlink, ca_link, link);
    /* TODO: -1 is correct way to signal error? */
    int ret = -1;

    epicsMutexMustLock(link->lock);
    /* channel connected, and first data update received */
    if(link->sub_value!=NULL && link->datavalid)
        ret = link->native_type;
    if(link->debug>1)
        errlogPrintf("%s -> %s: ca_link dtype=%d\n", link->plink->precord->name, link->target, ret);
    epicsMutexUnlock(link->lock);
    return ret;
}

static
long lnkca_elements(const struct link *plink, long *nelements)
{
    ca_link *link = CONTAINER(plink->value.json.jlink, ca_link, link);
    long ret = 0;

    epicsMutexMustLock(link->lock);
    /* channel connected, and first data update received */
    if(link->sub_value!=NULL && link->datavalid)
        ret = (long)link->getvalcount;
    if(link->debug>1)
        errlogPrintf("%s -> %s: ca_link elements=%ld\n", link->plink->precord->name, link->target, ret);
    epicsMutexUnlock(link->lock);
    return ret;
}

static
long lnkca_getValue(struct link *plink, short dbrType, void *pbuffer,
      long *pnRequest)
{
    /* note: dbrType is PDB (new) dtype code, link->*_code are CA (old) codes */
    long ret;
    ca_link *link = CONTAINER(plink->value.json.jlink, ca_link, link);
    long N = pnRequest ? *pnRequest : 1;
    unsigned srctype;

    epicsMutexMustLock(link->lock);
    srctype = dbDBRoldToDBFnew[link->native_type];

    if(link->sub_value==NULL || !link->datavalid) {
        ret = -1;

    } else if(N==0 || link->getvalcount==0) {
        N = 0;
        ret = 0;

    } else if(N==1) {
        long (*pgetconvert)(const void *from, void *to, dbAddr *paddr);

        pgetconvert = dbFastGetConvertRoutine[srctype][dbrType];

        ret = pgetconvert(link->getval, pbuffer, NULL);

    } else {
        struct dbAddr dbAddr;
        long (*aConvert)(struct dbAddr *paddr, void *to, long nreq, long nto, long off);

        if(N>link->getvalcount)
            N = link->getvalcount;

        aConvert = dbGetConvertRoutine[srctype][dbrType];

        memset(&dbAddr, 0, sizeof(dbAddr));
        dbAddr.pfield = link->getval;
        dbAddr.field_size = dbr_value_size[srctype];

        /*Ignore error return*/
        aConvert(&dbAddr, pbuffer, N, N, 0);
        ret = 0;
    }

    if(ret==0) {
        recGblInheritSevr(link->flags & pvlOptMsMode, plink->precord,
                          link->status, link->sevr);

        if(pnRequest) *pnRequest = N;
    }

    if(link->debug>1)
        errlogPrintf("%s -> %s: ca_link getValue code=%ld\n", link->plink->precord->name, link->target, ret);
    epicsMutexUnlock(link->lock);

    return ret;
}

static
long lnkca_controllimits(const struct link *plink, double *lo, double *hi)
{
    long ret;
    ca_link *link = CONTAINER(plink->value.json.jlink, ca_link, link);

    epicsMutexMustLock(link->lock);
    if(!link->metavalid) {
        ret = -1;
    } else {
        if(lo) *lo = link->meta.lower_ctrl_limit;
        if(hi) *hi = link->meta.upper_ctrl_limit;
        ret = 0;
    }
    if(link->debug>1)
        errlogPrintf("%s -> %s: ca_link controllimits\n", link->plink->precord->name, link->target);
    epicsMutexUnlock(link->lock);

    return ret;
}

static
long lnkca_graphiclimits(const struct link *plink, double *lo, double *hi)
{
    long ret;
    ca_link *link = CONTAINER(plink->value.json.jlink, ca_link, link);

    epicsMutexMustLock(link->lock);
    if(!link->metavalid) {
        ret = -1;
    } else {
        if(lo) *lo = link->meta.lower_disp_limit;
        if(hi) *hi = link->meta.upper_disp_limit;
        ret = 0;
    }
    if(link->debug>1)
        errlogPrintf("%s -> %s: ca_link graphiclimits\n", link->plink->precord->name, link->target);
    epicsMutexUnlock(link->lock);

    return ret;
}

static
long lnkca_alarmlimits(const struct link *plink, double *lolo, double *lo,
                       double *hi, double *hihi)
{
    long ret;
    ca_link *link = CONTAINER(plink->value.json.jlink, ca_link, link);

    epicsMutexMustLock(link->lock);
    if(!link->metavalid) {
        ret = -1;
    } else {
        if(lolo) *lolo = link->meta.lower_alarm_limit;
        if(lo) *lo = link->meta.lower_warning_limit;
        if(hi) *hi = link->meta.upper_warning_limit;
        if(hihi) *hihi = link->meta.upper_alarm_limit;
        ret = 0;
    }
    if(link->debug>1)
        errlogPrintf("%s -> %s: ca_link alarmlimits\n", link->plink->precord->name, link->target);
    epicsMutexUnlock(link->lock);

    return ret;
}

static
long lnkca_prec(const struct link *plink, short *precision)
{
    long ret;
    ca_link *link = CONTAINER(plink->value.json.jlink, ca_link, link);

    epicsMutexMustLock(link->lock);
    if(!link->metavalid) {
        ret = -1;
    } else {
        if(precision) *precision = link->meta.precision;
    }
    if(link->debug>1)
        errlogPrintf("%s -> %s: ca_link prec\n", link->plink->precord->name, link->target);
    epicsMutexUnlock(link->lock);

    return ret;
}

static
long lnkca_units(const struct link *plink, char *units, int unitsSize)
{
    long ret;
    ca_link *link = CONTAINER(plink->value.json.jlink, ca_link, link);

    epicsMutexMustLock(link->lock);
    if(!link->metavalid) {
        ret = -1;
    } else {
        if(unitsSize>MAX_UNITS_SIZE)
            unitsSize = MAX_UNITS_SIZE;
        strncpy(units, link->meta.units, unitsSize-1);
        units[unitsSize-1] = '\0';
    }
    if(link->debug>1)
        errlogPrintf("%s -> %s: ca_link egu\n", link->plink->precord->name, link->target);
    epicsMutexUnlock(link->lock);

    return ret;
}

static
long lnkca_alarm(const struct link *plink, epicsEnum16 *status,
                 epicsEnum16 *severity)
{
    ca_link *link = CONTAINER(plink->value.json.jlink, ca_link, link);
    epicsEnum16 sevr = INVALID_ALARM, sts = LINK_ALARM;

    epicsMutexMustLock(link->lock);
    if(link->datavalid) {
        sevr = link->sevr;
        sts  = link->status;
    }
    if(link->debug>1)
        errlogPrintf("%s -> %s: ca_link sevr=%u sts=%u\n", link->plink->precord->name, link->target, sevr, sts);
    epicsMutexUnlock(link->lock);

    if(status) *status = sts;
    if(severity) *severity = sevr;

    return 0;
}

static
long lnkca_time(const struct link *plink, epicsTimeStamp *pstamp)
{
    long ret = -1;
    ca_link *link = CONTAINER(plink->value.json.jlink, ca_link, link);

    epicsMutexMustLock(link->lock);
    if(link->datavalid) {
        ret = 0;
        if(pstamp)
            *pstamp = link->time;
    }
    if(link->debug>1)
        errlogPrintf("%s -> %s: ca_link sec=%u nsec=%u\n", link->plink->precord->name, link->target,
                     (unsigned)link->time.secPastEpoch,
                     (unsigned)link->time.nsec);
    epicsMutexUnlock(link->lock);

    return ret;
}

static
long lnkca_putValue(struct link *plink, short dbrType,
                    const void *pbuffer, long nRequest)
{
    int ret;
    struct ca_client_context *save;
    ca_link *link = CONTAINER(plink->value.json.jlink, ca_link, link);

    dbrType = dbDBRnewToDBRold[dbrType];

    save = ca_current_context();
    if(save)
        ca_detach_context();
    SEVCHK(ca_attach_context(lnkca_context),
           "open attach");

    epicsMutexMustLock(link->lock);

    if(link->channel) {

        ret = ca_array_put(dbrType, nRequest, link->channel, pbuffer);

        if(ret!=ECA_NORMAL) {
            errlogPrintf("%s: ca_link %s error (%d) %s\n",
                         link->plink->precord->name, link->target, ret, ca_message(ret));
        }
    } else {
        ret = ECA_DISCONN;
    }

    if(link->debug>2)
        errlogPrintf("%s -> %s: ca_link putval (%d) %s\n",
                     link->plink->precord->name, link->target, ret, ca_message(ret));

    epicsMutexUnlock(link->lock);

    if(ret==ECA_NORMAL) {
        SEVCHK(ca_flush_io(),
               "open flush");
    }

    ca_detach_context();
    if(save)
        ca_attach_context(save);

    return ret!=ECA_NORMAL;
}

static
long lnkca_putValueAsync(struct link *plink, short dbrType,
                         const void *pbuffer, long nRequest)
{
    return -1;
}

static lset lnkCalset = {
    0, 1, /* non-const, volatile */
    lnkca_open,
    lnkca_remove,
    NULL, NULL, NULL,
    lnkca_isconn,
    lnkca_dtype,
    lnkca_elements,
    lnkca_getValue,
    lnkca_controllimits,
    lnkca_graphiclimits,
    lnkca_alarmlimits,
    lnkca_prec,
    lnkca_units,
    lnkca_alarm,
    lnkca_time,
    lnkca_putValue,
    lnkca_putValueAsync,
    NULL
};

/* initialization */

static
void lnkca_exit(void *junk)
{
    /* TODO: locking?  links already closed? */
    struct ca_client_context *save = ca_current_context();
    if(save)
        ca_detach_context();
    if(lnkca_context) {
        ca_attach_context(lnkca_context);
        ca_context_destroy();
    }
    if(save)
        ca_attach_context(save);
}

static
void lnkca_init(void *junk)
{
    struct ca_client_context *save = ca_current_context();
    if(save)
        ca_detach_context();

    SEVCHK(ca_context_create(ca_enable_preemptive_callback),
           "ca_link context");

    SEVCHK(ca_add_exception_event(linkca_ca_error,NULL),
        "ca_add_exception_event");

    lnkca_context = ca_current_context();
    ca_detach_context();
    if(save)
        SEVCHK(ca_attach_context(save),
               "init attach");

    epicsAtExit(lnkca_exit, NULL);
}

/* jlink parsing */

static
jlink* lnkca_alloc(short dbfType)
{
    ca_link *link;

    epicsThreadOnce(&lnkca_once, lnkca_init, NULL);

    link = calloc(1, sizeof(*link));
    if(link) {
        epicsAtomicSetIntT(&link->refcount, 1);
        link->status = LINK_ALARM;
        link->sevr   = INVALID_ALARM;
        link->lock = epicsMutexCreate();
        if(!link->lock) {
            free(link);
            link = NULL;
        }
    }
    errlogPrintf("ca_link alloc\n");
    return link ? &link->link : NULL;
}

static
void lnkca_free(jlink *lnk)
{
    ca_link *link = CONTAINER(lnk, ca_link, link);
    assert(epicsAtomicGetIntT(&link->refcount)>0);
    if(epicsAtomicDecrIntT(&link->refcount)==0) {
        if(link->debug>2)
            errlogPrintf("ca_link free\n");
        epicsMutexDestroy(link->lock);
        free(link->target);
        free(link);
    }
}

static
jlif_key_result linkca_start_map(jlink *lnk)
{
    if(lnk->parseDepth==1) {
        return jlif_key_continue;
    } else {
        return jlif_key_stop;
    }
}

static
jlif_result linkca_key(jlink *lnk, const char *key, size_t len)
{
    ca_link *link = CONTAINER(lnk, ca_link, link);

    if(len==2 && strncmp("pv", key, len)==0) {
        link->pstate = lnkCaParsePV;
        return jlif_key_continue;

    } else if(len==2 && strncmp("ms", key, len)==0) {
        link->pstate = lnkCaParseMS;
        return jlif_key_continue;

    } else if(len==4 && strncmp("proc", key, len)==0) {
        link->pstate = lnkCaParsePROC;
        return jlif_key_continue;

    } else if(len==5 && strncmp("debug", key, len)==0) {
        link->pstate = lnkCaParseDEBUG;
        return jlif_key_continue;

    } else if(len==5 && strncmp("retry", key, len)==0) {
        link->pstate = lnkCaParseQOS;
        return jlif_key_continue;

    } else {
        return jlif_key_stop;
    }
}

static
jlif_result linkca_end_map(jlink *lnk)
{
    return jlif_key_continue;
}

static
jlif_result linkca_bool(jlink *lnk, int val)
{
    ca_link *link = CONTAINER(lnk, ca_link, link);

    switch(link->pstate) {
    case lnkCaParseDEBUG:
        link->debug = !!val;
        break;
    case lnkCaParseMS:
        link->flags &= ~pvlOptMsMode;
        if(val)
            link->flags |= pvlOptMS;
        else
            link->flags |= pvlOptNMS;
        break;
    case lnkCaParsePROC:
        link->flags &= ~(pvlOptPP|pvlOptCA|pvlOptCP|pvlOptCPP);
        if(val)
            link->flags |= pvlOptPP;
        break;
    default:
        return jlif_stop;
    }
    return jlif_key_continue;
}

static
jlif_result linkca_int(jlink *lnk, long val)
{
    ca_link *link = CONTAINER(lnk, ca_link, link);

    switch(link->pstate) {
    case lnkCaParseDEBUG:
        if(val>3) val = 3;
        link->debug = val;
        break;
    default:
        return jlif_stop;
    }
    return jlif_key_continue;
}

static
jlif_result linkca_parse_string(jlink *lnk, const char *val, size_t len)
{
    ca_link *link = CONTAINER(lnk, ca_link, link);

    if(link->pstate == lnkCaParsePV) {
        /* TODO: strip whitespace? */
        char *buf = malloc(len+1);
        if(!buf) {
            return jlif_stop;
        }
        memcpy(buf, val, len);
        buf[len] = '\0';
        free(link->target);
        link->target = buf;
        return jlif_continue;

    } else if(link->pstate == lnkCaParseMS) {
        link->flags &= ~pvlOptMsMode;
        if(len==1 && val[0]=='I')
            link->flags |= pvlOptMSI;
        else if(len==1 && val[0]=='S')
            link->flags |= pvlOptMSS;
        else
            return jlif_stop; /* TODO strict? */

        return jlif_continue;

    } else if(link->pstate == lnkCaParsePROC) {
        link->flags &= ~(pvlOptPP|pvlOptCA|pvlOptCP|pvlOptCPP);
        if(len==2 && val[0]=='C' && val[1]=='P')
            link->flags |= pvlOptCP;
        else if(len==3 && val[0]=='C' && val[1]=='P' && val[2]=='P')
            link->flags |= pvlOptCPP;
        else
            return jlif_stop; /* TODO strict? */
        return jlif_continue;

    } else if(link->pstate == lnkCaParseQOS) {
        if(len==5 && strncmp(val, "retry", 5)==0) {

        }
        else
            return jlif_stop; /* TODO strict? */
        return jlif_continue;

    } else {
        return jlif_stop;
    }
}

static
struct lset* lnkca_get_lset(const jlink *link)
{
    errlogPrintf("ca_link get lset\n");
    return &lnkCalset;
}

static
void lnkca_report(const jlink *lnk, int level, int indent)
{
    ca_link *link = CONTAINER(lnk, ca_link, link);

    epicsMutexMustLock(link->lock);

    printf("%*s'ca': \"%s\" %sdata %smeta\n", indent, "",
           link->target ? link->target : "<missing>",
           link->datavalid ? "" : "no ",
           link->metavalid ? "" : "no ");

    if(level>0) {

    }

    epicsMutexUnlock(link->lock);
}

static jlif lnkCAIf = {
    "ca",
    lnkca_alloc,
    lnkca_free,
    NULL,
    linkca_bool,
    linkca_int,
    NULL,
    linkca_parse_string,
    linkca_start_map,
    linkca_key,
    linkca_end_map,
    NULL,
    NULL,
    NULL,
    lnkca_get_lset,
    lnkca_report,
    NULL,
};
epicsExportAddress(jlif, lnkCAIf);
