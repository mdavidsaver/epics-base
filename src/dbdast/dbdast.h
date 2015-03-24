#ifndef DBDAST_H
#define DBDAST_H

#ifdef __cplusplus
#include <istream>
#endif

#include <stdio.h>

#include <ellLib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DBDContext DBDContext;

typedef struct DBDNode DBDNode;

typedef struct DBDFile DBDFile;
typedef struct DBDNest DBDNest;
typedef struct DBDStatement DBDStatement;
typedef struct DBDBlock DBDBlock;

typedef enum {
    DBDNodeFile,
    DBDNodeNest,
    DBDNodeBlock,
    DBDNodeStatement
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

void DBDPathSplit(const char *str,
                  void (*cb)(void*, const char*, size_t),
                  void* arg);

#define DBDContext_NO_DEFAULT_PATH 1
#define DBDContext_NO_INCLUDE 2

DBDContext *DBDContextCreate(unsigned flags);
void DBDContextDestroy(DBDContext*);

void DBDContextSetPaths(DBDContext*, const char*);
void DBDContextAddPath(DBDContext*, const char*);

/* Returns a string which must be free'd */
const char* DBDContextFindFile(DBDContext*, const char*);

DBDFile *DBDParseFile(DBDContext *ctxt, const char* fname);
DBDFile *DBDParseFileP(DBDContext *ctxt, FILE *, const char* fname);
DBDFile *DBDParseMemory(DBDContext *ctxt, const char *buf, const char *fname);

#ifdef __cplusplus
DBDFile *DBDParseStream(DBDContext *ctxt, std::istream&, const char *fname);
#endif

/* free pointer to DBDFile, DBDNest, DBDBlock, DBDStatement, or DBDValue */
#define DBDFree(pnode) DBDFreeNode(&(pnode)->common)
void DBDFreeNode(DBDNode *node);

void DBDShow(const DBDNode *node, FILE *fp, int indent);

#ifdef __cplusplus
}
#endif

#endif /* DBDAST_H */
