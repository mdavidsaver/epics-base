#ifndef EPICSJTREEPVT_H
#define EPICSJTREEPVT_H

#include <ellLib.h>

#include "epicsjtree.h"

#ifdef __cplusplus
extern "C" {
#endif

union jStore {
    int jbool;
    long long jint;
    double jreal;
    const char* jstr;
    ELLLIST jlist; /* list of nodes in map or array */
};

struct epicsJNode {
    ELLNODE node;
    epicsJNodeType type;
    const char* key;
    union jStore store;
};

struct epicsJTree {
    size_t ncnt;
    char *strbuf;
    epicsJNode nodes[1]; /* actually longer */
};

#ifdef __cplusplus
}
#endif

#endif // EPICSJTREEPVT_H
