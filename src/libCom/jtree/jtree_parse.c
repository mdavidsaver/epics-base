/*************************************************************************\
* Copyright (c) 2018 Michael Davidsaver
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <stdlib.h>
#include <string.h>

#define epicsExportSharedSymbols
#include <yajl_parse.h>
#include <epicsAssert.h>

#include "jtreepvt.h"

/* To be super super memory efficient, we parse twice.
 * First to count the number of nodes, then again to store values
 */

typedef struct {
    size_t ncnt; /* number of nodes */
    size_t maxdepth; /* maximum nesting depth */
    size_t curdepth;
    size_t nchars; /* number of string charactors */
} stageone;

#define CTXT stageone *context = ctx

static
int stageone_null(void *ctx) {
    CTXT;
    if(context->ncnt==(size_t)-1) return 0;
    context->ncnt++;
    return 1;
}

static
int stageone_bool(void *ctx, int val) {
    CTXT;
    if(context->ncnt==(size_t)-1) return 0;
    context->ncnt++;
    return 1;
}

static
int stageone_number(void *ctx, const char * val, size_t len) {
    CTXT;
    if(context->ncnt==(size_t)-1) return 0;
    context->ncnt++;
    return 1;
}

static
int stageone_string(void *ctx, const unsigned char * val, size_t len) {
    CTXT;
    if(context->ncnt==(size_t)-1) return 0;
    context->ncnt++;
    if(len==(size_t)-1 || context->nchars >= ((size_t)-1)-len) return 0;
    context->nchars += len+1;
    return 1;
}

static
int stageone_start_map_array(void *ctx) {
    CTXT;
    if(context->ncnt==(size_t)-1) return 0;
    context->ncnt++;
    if(context->curdepth==(size_t)-1) return 0;
    context->curdepth++;
    if(context->maxdepth < context->curdepth)
        context->maxdepth = context->curdepth;
    return 1;
}

static
int stageone_key(void *ctx, const unsigned char * val, size_t len) {
    CTXT;
    if(len==(size_t)-1 || context->nchars >= ((size_t)-1)-len) return 0;
    context->nchars += len+1;
    return 1;
}

static
int stageone_end_map_array(void *ctx) {
    CTXT;
    context->curdepth--;
    return 1;
}

static const yajl_callbacks stageone_callbacks = {
    stageone_null,
    stageone_bool,
    NULL,
    NULL,
    stageone_number,
    stageone_string,
    stageone_start_map_array,
    stageone_key,
    stageone_end_map_array,
    stageone_start_map_array,
    stageone_end_map_array,
};


typedef struct {
    size_t curdepth, maxdepth;
    size_t nextfree, maxfree;
    size_t sbuflen;

    char *sbuf;
    epicsJNode **stack, *nodes;

    const char* key;

    void (*errfn)(void *errpvt, size_t pos, const char *msg);
    void *errpvt;
} stagetwo;

#undef CTXT
#define CTXT stagetwo *context = ctx

const char* alloc_string(stagetwo *context, const char *buf, size_t len)
{
    char *ret = context->sbuf;
    assert(context->sbuflen >= len+1);

    memcpy(ret, buf, len);
    ret[len] = '\0';

    context->sbuf += len+1;
    context->sbuflen -= len+1;

    return ret;
}

epicsJNode* alloc_node(stagetwo *context, epicsJNodeType type)
{
    epicsJNode *ret;
    epicsJNode *parent = context->stack[context->curdepth];

    assert(context->nextfree < context->maxfree);
    assert(context->curdepth <= context->maxdepth);

    ret = &context->nodes[context->nextfree++];
    ret->type = type;

    if(context->key) {
        ret->key = context->key;
        context->key = 0;
    }

    if(parent) {
        /* add ourselves to parent node */
        assert(parent->type == epicsJNodeTypeMap || parent->type == epicsJNodeTypeArray);

        ellAdd(&parent->store.jlist, &ret->node);
    }

    if(ret->type == epicsJNodeTypeMap || ret->type == epicsJNodeTypeArray) {
        assert(context->curdepth < context->maxdepth);

        context->stack[++context->curdepth] = ret;
    }

    return ret;
}

static
int stagetwo_null(void *ctx) {
    CTXT;
    alloc_node(context, epicsJNodeTypeNull);
    return 1;
}

static
int stagetwo_bool(void *ctx, int val) {
    CTXT;
    epicsJNode *node = alloc_node(context, epicsJNodeTypeBool);
    node->store.jbool = val;
    return 1;
}

static
int stagetwo_integer(void *ctx, long long val) {
    CTXT;
    epicsJNode *node = alloc_node(context, epicsJNodeTypeInteger);
    node->store.jint = val;
    return 1;
}

static
int stagetwo_double(void *ctx, double val) {
    CTXT;
    epicsJNode *node = alloc_node(context, epicsJNodeTypeReal);
    node->store.jreal = val;
    return 1;
}

static
int stagetwo_string(void *ctx, const unsigned char * val, size_t len) {
    CTXT;
    epicsJNode *node = alloc_node(context, epicsJNodeTypeString);
    node->store.jstr = alloc_string(context, (const char*)val, len);
    return 1;
}

static
int stagetwo_start_map(void *ctx) {
    CTXT;
    alloc_node(context, epicsJNodeTypeMap);
    return 1;
}

static
int stagetwo_map_key(void *ctx, const unsigned char * val, size_t len) {
    CTXT;
    assert(!context->key);
    context->key = alloc_string(context, (const char*)val, len);
    return 1;
}

static
int stagetwo_end_map(void *ctx) {
    CTXT;
    assert(context->curdepth > 0);
    assert(context->stack[context->curdepth]->type == epicsJNodeTypeMap);
    context->stack[context->curdepth--] = NULL;
    return 1;
}

static
int stagetwo_start_array(void *ctx) {
    CTXT;
    alloc_node(context, epicsJNodeTypeArray);
    return 1;
}

static
int stagetwo_end_array(void *ctx) {
    CTXT;
    assert(context->curdepth > 0);
    assert(context->stack[context->curdepth]->type == epicsJNodeTypeArray);
    context->stack[context->curdepth--] = NULL;
    return 1;
}

static const yajl_callbacks stagetwo_callbacks = {
    stagetwo_null,
    stagetwo_bool,
    stagetwo_integer,
    stagetwo_double,
    NULL,
    stagetwo_string,
    stagetwo_start_map,
    stagetwo_map_key,
    stagetwo_end_map,
    stagetwo_start_array,
    stagetwo_end_array,
};

#undef CTXT

const epicsJTree* ejtParse(const char *inp, size_t inplen,
                                  void (*errfn)(void *errpvt, size_t pos, const char *msg),
                                  void *errpvt,
                                  unsigned flags)
{
    stageone ctxt1;
    stagetwo ctxt2;
    epicsJTree* ret = NULL;

    yajl_handle handle = NULL;
    yajl_status status;

    memset(&ctxt1, 0, sizeof(ctxt1));
    memset(&ctxt2, 0, sizeof(ctxt2));

    /* stage one, count nodes */

    handle = yajl_alloc(&stageone_callbacks, NULL, &ctxt1);
    if(!handle)
        goto internal;

    yajl_config(handle, yajl_allow_comments, 1);

    status = yajl_parse(handle, (const unsigned char *)inp, inplen);

    if(status == yajl_status_ok)
        status = yajl_complete_parse(handle);

    switch(status) {
    case yajl_status_ok:
        break;
    case yajl_status_client_canceled:
        if(errfn) (*errfn)(errpvt, yajl_get_bytes_consumed(handle), "Input too large");
        goto done;
    case yajl_status_error:
        /* this is where we find out about syntax errors */
        if(errfn) {
            char *msg = (char*)yajl_get_error(handle, 1, (const unsigned char *)inp, inplen);
            (*errfn)(errpvt, yajl_get_bytes_consumed(handle), msg);
            yajl_free_error(handle, (unsigned char*)msg);
        }
        goto done;
    }

    if(ctxt1.ncnt==0) {
        if(errfn) {
            (*errfn)(errpvt, yajl_get_bytes_consumed(handle), "No JSON found");
        }
        goto done;
    }

    yajl_free(handle);
    handle = NULL;

    assert(ctxt1.curdepth == 0);

    /* stage two, build tree */

    ret = calloc(1, sizeof(*ret) + sizeof(ret->nodes)*(ctxt1.ncnt-1));
    if(!ret)
        goto internal;

    ret->strbuf = malloc(ctxt1.nchars);
    if(ctxt1.nchars!=0 && !ret->strbuf)
        goto internal;

    ctxt2.nodes = ret->nodes;
    ctxt2.sbuf = ret->strbuf;
    ctxt2.sbuflen = ctxt1.nchars;
    ctxt2.maxdepth = 1+ctxt1.maxdepth; /* one extra slot for top node */
    ctxt2.maxfree = ret->ncnt = ctxt1.ncnt;
    ctxt2.errfn = errfn;
    ctxt2.errpvt = errpvt;

    ctxt2.stack = calloc(ctxt2.maxdepth, sizeof(*ctxt2.stack));
    if(ctxt2.maxdepth!=0 && !ctxt2.stack)
        goto internal;

    handle = yajl_alloc(&stagetwo_callbacks, NULL, &ctxt2);
    if(!handle)
        goto internal;

    yajl_config(handle, yajl_allow_comments, 1);

    status = yajl_parse(handle, (const unsigned char *)inp, inplen);

    if(status == yajl_status_ok)
        status = yajl_complete_parse(handle);

    switch(status) {
    case yajl_status_ok:
        break;
    case yajl_status_client_canceled:
        /* callbbacks have already called errfn() */
        goto done;
    case yajl_status_error:
        /* stage one should have found any syntax errors */
        goto internal;
    }

    yajl_free(handle);
    handle = NULL;
    free(ctxt2.stack);
    ctxt2.stack = NULL;

    assert(ctxt2.curdepth == 0); /* top node left on stack */
    assert(ctxt2.nextfree == ctxt2.maxfree);
    assert(ctxt2.sbuflen == 0);

    assert(ctxt2.maxfree==1 || ret->nodes[0].type==epicsJNodeTypeMap || ret->nodes[0].type==epicsJNodeTypeArray);

    return ret;
internal:
    if(errfn) (*errfn)(errpvt, 0, "Internal parser error");
done:
    if(handle) yajl_free(handle);
    free(ctxt2.stack);
    ejtFree(ret);
    return NULL;
}

void ejtFree(const epicsJTree* tree)
{
    epicsJTree* mtree = (epicsJTree*)tree;
    if(mtree) {
        free(mtree->strbuf);
    }
    free(mtree);
}

void ejtPos2Line(const char *inp, size_t inplen, size_t pos, size_t *line, size_t *col)
{
    size_t i, N = pos <= inplen ? pos : inplen,
           L = 1, C = 1;

    for(i=0; i<N; i++) {
        if(inp[i]=='\n') {
            L++;
            C = 1;
        } else {
            C++;
        }
    }

    if(line)
        *line = L;
    if(col)
        *col = C;
}
