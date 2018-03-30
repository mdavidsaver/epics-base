/*************************************************************************\
* Copyright (c) 2018 Michael Davidsaver
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/

#include <stdlib.h>
#include <string.h>

#define epicsExportSharedSymbols
#include <epicsStdlib.h>
#include <epicsStdio.h>
#include <dbDefs.h>
#include <yajl_parse.h>
#include <epicsAssert.h>

#include "jtreepvt.h"

const epicsJNode* ejtRoot(const epicsJTree* tree)
{
    if(tree && tree->ncnt) {
        return &tree->nodes[0];
    } else {
        return NULL;
    }
}

epicsJNodeType ejtType(const epicsJNode* node)
{
    if(node)
        return node->type;
    else
        return epicsJNodeTypeNull;
}

size_t ejtCount(const epicsJNode* node)
{
    if(node && (node->type == epicsJNodeTypeArray || node->type == epicsJNodeTypeMap)) {
        return ellCount(&node->store.jlist);
    } else {
        return (size_t)-1; /* undefined */
    }
}

const epicsJNode* ejtLookup(const epicsJNode* node, const char *key, unsigned flags)
{
    ELLNODE *cur;
    const ELLLIST *list;
    if(!node || !key) return NULL;

    list = &node->store.jlist;

    for(cur = ellFirst(list); cur; cur = ellNext(cur)) {
        epicsJNode *child = CONTAINER(cur, epicsJNode, node);
        if(strcmp(child->key, key)==0) {
            return child;
        }
    }
    return NULL;
}

const epicsJNode* ejtIndex(const epicsJNode* node, size_t index)
{
    const epicsJNode *ret = NULL;
    if(node && (node->type == epicsJNodeTypeMap || node->type == epicsJNodeTypeArray)) {
        const ELLLIST *list = &node->store.jlist;
        ELLNODE *elem = ellNth((ELLLIST*)list, index+1);

        if(elem) {
            ret = CONTAINER(elem, epicsJNode, node);
        }
    }
    return ret;
}

int ejtAsBool(const epicsJNode* node, unsigned flags, int *result)
{
    if(!node) {
        return 0;
    } else if(node->type == epicsJNodeTypeBool) {
        *result = !!node->store.jbool;
        return 1;
    } else if(flags&EJT_VAL_ERROR) {
        return 0;
    } else switch(node->type) {
    case epicsJNodeTypeNull:
        /* NULL is false */
        *result = 0;
        return 1;
    case epicsJNodeTypeInteger:
        /* zero / non-zero */
        *result = !!node->store.jint;
        return 1;
    case epicsJNodeTypeReal:
        /* false is [0, 0.5) */
        *result = node->store.jreal < 0.0 || node->store.jreal >= 0.5;
        return 1;
    case epicsJNodeTypeString:
        /* match "true" or "false" */
        if(epicsStrCaseCmp(node->store.jstr, "true")==0) {
            *result = 1;
            return 1;
        } else if(epicsStrCaseCmp(node->store.jstr, "false")==0) {
            *result = 0;
            return 1;
        } else {
            return 0;
        }
    default:
        return 0;
    }
}

int ejtAsInteger(const epicsJNode* node, unsigned flags, long long *result)
{
    if(!node) {
        return 0;
    } else if(node->type == epicsJNodeTypeInteger) {
        *result = node->store.jint;
        return 1;
    } else if(flags&EJT_VAL_ERROR) {
        return 0;
    } else switch(node->type) {
    case epicsJNodeTypeNull:
        /* NULL is zero */
        *result = 0;
        return 1;
    case epicsJNodeTypeBool:
        /* 1 or 0 */
        *result = !!node->store.jbool;
        return 1;
    case epicsJNodeTypeReal:
        /* truncate (round to zero) */
        *result = node->store.jreal;
        return 1;
    case epicsJNodeTypeString:
        return !epicsParseLLong(node->store.jstr, result, 0, NULL); /* 0 on sucess */
    default:
        return 0;
    }
}

int ejtAsReal(const epicsJNode* node, unsigned flags, double *result)
{
    if(!node) {
        return 0;
    } else if(node->type == epicsJNodeTypeReal) {
        *result = node->store.jreal;
        return 1;
    } else if(flags&EJT_VAL_ERROR) {
        return 0;
    } else switch(node->type) {
    case epicsJNodeTypeNull:
        /* NULL is zero */
        *result = 0.0;
        return 1;
    case epicsJNodeTypeBool:
        /* 1 or 0 */
        *result = !!node->store.jbool;
        return 1;
    case epicsJNodeTypeInteger:
        /* truncate (round to zero) */
        *result = node->store.jint;
        return 1;
    case epicsJNodeTypeString:
        return !epicsParseDouble(node->store.jstr, result, NULL); /* 0 on sucess */
    default:
        return 0;
    }
}

int ejtAsString(const epicsJNode* node, unsigned flags, const char* *result)
{
    if(!node) {
        return 0;
    } else if(node->type == epicsJNodeTypeString) {
        *result = node->store.jstr;
        return 1;
    }
    /* can't allocate here, so no conversions possible */
    return 0;
}


char *ejtAsStringAlloc(const epicsJNode* node, unsigned flags)
{
    if(!node) {
        return NULL;
    } else if(node->type == epicsJNodeTypeString) {
        return epicsStrDup(node->store.jstr);
    } else if(flags&EJT_VAL_ERROR) {
        return NULL;
    } else switch(node->type) {
    case epicsJNodeTypeNull:
        return epicsStrDup("<NULL>");
    case epicsJNodeTypeBool:
        return epicsStrDup(node->store.jbool ? "true" : "false");
    case epicsJNodeTypeInteger: {
        char *buf = malloc(20);
        if(buf) {
            epicsSnprintf(buf, 20, "%lld", node->store.jint);
            buf[19] = '\0';
        }
        return buf;
    }
    case epicsJNodeTypeReal: {
        char *buf = malloc(20);
        if(buf) {
            epicsSnprintf(buf, 20, "%g", node->store.jreal);
            buf[19] = '\0';
        }
        return buf;
    }
    default:
        return NULL;
    }
}

int ejtIterate(const epicsJNode* rawnode, epicsJNodeIterator* iter, const char** key, const epicsJNode** child)
{
    epicsJNode* node = (epicsJNode*)rawnode;

    if(!node || !iter || (node->type!=epicsJNodeTypeArray && node->type!=epicsJNodeTypeMap)) {
        return 0;
    } else if(!*iter) {
        *iter = ellFirst(&node->store.jlist);
    } else {
        *iter = ellNext(*iter);
    }

    if(*iter) {
        epicsJNode *cur = CONTAINER(*iter, epicsJNode, node);
        if(key)
            *key = cur->key;
        if(child)
            *child = cur;
    }
    return !!*iter;
}
