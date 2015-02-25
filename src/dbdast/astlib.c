
#include <stdlib.h>

#include "dbDefs.h"

#include "dbdast.h"

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
    case DBDNodeValue:
        fprintf(fp, "### bare value %s !!!\n", ((const DBDValue*)node)->value);
        break;
    default:
        fprintf(fp, "### Unknown node type %d !!!\n", node->type);
    }
}
