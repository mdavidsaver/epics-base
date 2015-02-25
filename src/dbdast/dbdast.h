#ifndef DBDAST_H
#define DBDAST_H

#include <stdio.h>

#include <ellLib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DBDNode DBDNode;

typedef struct DBDFile DBDFile;
typedef struct DBDNest DBDNest;
typedef struct DBDStatement DBDStatement;
typedef struct DBDBlock DBDBlock;
typedef struct DBDValue DBDValue;

typedef enum {
    DBDNodeFile,
    DBDNodeNest,
    DBDNodeBlock,
    DBDNodeStatement,
    DBDNodeValue,
} DBDNodeType;

struct DBDNode {
    ELLNODE node;
    DBDNode *parent; /* DBDFile or DBDBlock */
    DBDNodeType type;
    unsigned line, col;
};

/* top level */
struct DBDFile {
    DBDNode common;
    ELLLIST entries;
    char name[1];
};

struct DBDNest {
    DBDNode common;
    char line[1];
};

#define DBDBlockMaxArgs 16

struct DBDBlock {
    DBDNode common;
    ELLLIST children;
    unsigned nargs;
    char *name, *args[DBDBlockMaxArgs];

    char _alloc[1]; /* buffer for name and args */
};

struct DBDStatement {
    DBDNode common;
    char *cmd, *arg;

    char _alloc[1]; /* buffer for name and arg */
};

struct DBDValue {
    DBDNode common;
    char value[1];
};

DBDFile *DBDParseFile(const char* fname);
DBDFile *DBDParseFileP(FILE *, const char* fname);
DBDFile *DBDParseMemory(const char *buf, const char *fname);

/* free pointer to DBDFile, DBDNest, DBDBlock, DBDStatement, or DBDValue */
#define DBDFree(pnode) DBDFreeNode(&(pnode)->common)
void DBDFreeNode(DBDNode *node);

void DBDShow(const DBDNode *node, FILE *fp, int indent);

#ifdef __cplusplus
}
#endif

#endif /* DBDAST_H */
