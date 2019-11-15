/*************************************************************************\
* Copyright (c) 2010 UChicago Argonne LLC, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 *      Author: John Winans (ANL)
 *              Andrew Johnson <anj@aps.anl.gov>
 */
#ifndef INC_ellLib_H
#define INC_ellLib_H

#include "libComAPI.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ELLNODE {
    struct ELLNODE *next;
    struct ELLNODE *previous;
} ELLNODE;

#define ELLNODE_INIT {NULL, NULL}

typedef struct ELLLIST {
    ELLNODE node;
    int     count;
} ELLLIST;

#define ELLLIST_INIT {ELLNODE_INIT, 0}

typedef void (*FREEFUNC)(void *);

#define ellInit(PLIST) {\
    (PLIST)->node.next = (PLIST)->node.previous = NULL;\
    (PLIST)->count = 0;\
}
#define ellCount(PLIST)    ((PLIST)->count)
#define ellFirst(PLIST)    ((PLIST)->node.next)
#define ellLast(PLIST)     ((PLIST)->node.previous)
#define ellNext(PNODE)     ((PNODE)->next)
#define ellPrevious(PNODE) ((PNODE)->previous)
#define ellFree(PLIST)     ellFree2(PLIST, free)

LIBCOM_API void ellAdd (ELLLIST *pList, ELLNODE *pNode);
LIBCOM_API void ellConcat (ELLLIST *pDstList, ELLLIST *pAddList);
LIBCOM_API void ellDelete (ELLLIST *pList, ELLNODE *pNode);
LIBCOM_API void ellExtract (ELLLIST *pSrcList, ELLNODE *pStartNode, ELLNODE *pEndNode, ELLLIST *pDstList);
LIBCOM_API ELLNODE * ellGet (ELLLIST *pList);
LIBCOM_API ELLNODE * ellPop (ELLLIST *pList);
LIBCOM_API void ellInsert (ELLLIST *plist, ELLNODE *pPrev, ELLNODE *pNode);
LIBCOM_API ELLNODE * ellNth (ELLLIST *pList, int nodeNum);
LIBCOM_API ELLNODE * ellNStep (ELLNODE *pNode, int nStep);
LIBCOM_API int  ellFind (ELLLIST *pList, ELLNODE *pNode);
typedef int (*pListCmp)(const ELLNODE* A, const ELLNODE* B);
LIBCOM_API void ellSortStable(ELLLIST *pList, pListCmp);
LIBCOM_API void ellFree2 (ELLLIST *pList, FREEFUNC freeFunc);
LIBCOM_API void ellVerify (ELLLIST *pList);

#ifdef __cplusplus
}
#endif

#endif /* INC_ellLib_H */
