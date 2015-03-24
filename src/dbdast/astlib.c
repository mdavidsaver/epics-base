
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dbDefs.h"
#include "epicsAssert.h"
#include "osiFileName.h"

#include "dbdastPvt.h"

STATIC_ASSERT(NELEMENTS(OSI_PATH_LIST_SEPARATOR)==2);

void DBDPathSplit(const char *str,
                      void (*cb)(void*, const char*, size_t),
                      void* arg)
{
    while(*str)
    {
        const char *mark;

        // skip space
        while(*str && isspace(*str)) str++;
        if(!*str) break;

        mark = str; // mark start of token
        // consume token until space or path sep
        while(*str && *str!=OSI_PATH_LIST_SEPARATOR[0] && !isspace(*str)) str++;

        if(mark!=str)
            cb(arg, mark, str-mark);
    }
}

static void catpath(void *arg, const char *start, size_t len)
{
    DBDContext *ctxt=arg;
    DBDPathNode *node = calloc(1, sizeof(*node)+len);
    if(!node)
        return;
    memcpy(node->path, start, len);
    ellAdd(&ctxt->paths, &node->node);
}

DBDContext* DBDContextCreate(unsigned flags)
{
    DBDContext *ctxt = calloc(1, sizeof(*ctxt));
    if(!ctxt)
        return ctxt;

    if(!(flags&DBDContext_NO_DEFAULT_PATH))
    {
        char *env = getenv("EPICS_DB_INCLUDE_PATH");
        if(!env)
            catpath(ctxt, ".", 1);
        else {
            DBDPathSplit(env, &catpath, ctxt);
        }
    }
    ctxt->doinclude = !(flags&DBDContext_NO_INCLUDE);
    return ctxt;
}

void
DBDContextDestroy(DBDContext *ctxt)
{
    ellFree(&ctxt->paths);
    free(ctxt);
}

void DBDContextSetPaths(DBDContext *ctxt, const char* str)
{
    ellFree(&ctxt->paths);
    DBDPathSplit(str, &catpath, ctxt);
}

void DBDContextAddPath(DBDContext *ctxt, const char* str)
{
    DBDPathSplit(str, &catpath, ctxt);
}

const char* DBDContextFindFile(DBDContext *ctxt, const char *name)
{
    ELLNODE *cur;
    size_t namelen = strlen(name), buflen=0;
    char *buf=NULL;

    for(cur=ellFirst(&ctxt->paths); cur; cur=ellNext(cur))
    {
        char *scratch;
        DBDPathNode *node = CONTAINER(cur, DBDPathNode, node);
        /* NELEMENTS(...) includes space for nil */
        size_t dirlen = strlen(node->path), totlen=dirlen+namelen+NELEMENTS(OSI_PATH_LIST_SEPARATOR);

        if(!buf || buflen<totlen) {
            buf = realloc(buf, totlen);
            if(!buf)
                buf = malloc(totlen);
            if(!buf)
                return NULL;
        }

        scratch = buf;
        memcpy(scratch, node->path, dirlen);

        scratch += dirlen;
        strcpy(scratch, OSI_PATH_LIST_SEPARATOR);

        scratch += NELEMENTS(OSI_PATH_LIST_SEPARATOR);
        memcpy(scratch, name, namelen);

        scratch += namelen;
        *scratch = '\0';

        assert((scratch-buf-1)==totlen);

        FILE *fp=fopen(buf, "r");
        if(fp) {
            fclose(fp);
            return buf; /* caller must free */
        }
    }
    free(buf);
    return NULL;
}

static void DBDFreeFile(DBDFile *node)
{
    ELLNODE *cur;
    while( (cur=ellGet(&node->entries))!=NULL) {
        DBDFreeNode((DBDNode*)cur);
    }
}

static void DBDFreeBlock(DBDBlock *node)
{
    ELLNODE *cur;
    while( (cur=ellGet(&node->children))!=NULL) {
        DBDFreeNode((DBDNode*)cur);
    }
}

void DBDFreeNode(DBDNode *node)
{
    switch(node->type) {
        case DBDNodeFile: DBDFreeFile((DBDFile*)node); break;
        case DBDNodeBlock: DBDFreeBlock((DBDBlock*)node); break;
        default: break;
    }
    free(node);
}

void DBDFreeInt(void *node, void *junk)
{
    DBDFreeNode((DBDNode*)node);
}

static void DBDindent(FILE *fp, int N)
{
    N *= 2;
    while(N--) fputc(' ', fp);
}

static void DBDshowall(const ELLLIST *list, FILE *fp, int indent)
{
    ELLNODE *cur;
    for(cur=ellFirst(list); cur; cur=ellNext(cur)) {
        DBDNode *node = CONTAINER(cur, DBDNode, node);
        DBDShow(node, fp, indent);
    }
}

void DBDShow(const DBDNode *node, FILE *fp, int indent)
{
    DBDindent(fp, indent);

    switch(node->type) {
    case DBDNodeFile: {
        const DBDFile *file=(DBDFile*)node;
        if(file->name[0])
            fprintf(fp, "# file: %s\n", file->name);
        DBDshowall(&file->entries, fp, indent);
    } break;
    case DBDNodeNest: {
        const DBDNest *nest=(DBDNest*)node;
        fprintf(fp, "%s\n", nest->line);
    } break;
    case DBDNodeBlock: {
        const DBDBlock *blk=(DBDBlock*)node;
        unsigned i;
        fprintf(fp, "%s(", blk->name);
        for(i=0; i<blk->nargs; i++) {
            if(i>0)
                fputs(", ", fp);
            fprintf(fp, "%s", blk->args[i]);
        }
        fprintf(fp, ")");
        if(ellCount(&blk->children)>0) {
            fprintf(fp, " {\n");
            DBDshowall(&blk->children, fp, indent+2);
            DBDindent(fp, indent);
            fprintf(fp, "}");
        }
        fprintf(fp, "\n");
    } break;
    case DBDNodeStatement: {
        const DBDStatement *stmt=(DBDStatement*)node;
        fprintf(fp, "%s %s\n", stmt->cmd, stmt->arg);
    } break;
    default:
        fprintf(fp, "### Unknown node type %d !!!\n", node->type);
    }
}
