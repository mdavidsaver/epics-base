#ifndef DBDASTPVT_H
#define DBDASTPVT_H

#include "dbdast.h"

typedef struct {
    ELLNODE node;
    char path[1];
} DBDPathNode;

struct DBDContext {
    ELLLIST paths;
    unsigned int doinclude:1;
};

#endif // DBDASTPVT_H
