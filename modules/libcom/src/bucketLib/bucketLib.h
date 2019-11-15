/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE is distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/
/*
 *      Author: Jeffrey O. Hill
 *              hill@luke.lanl.gov
 *              (505) 665 1831
 *      Date:  9-93 
 *
 *	NOTES:
 *	.01 Storage for identifier must persist until an item is deleted
 */

#ifndef INCbucketLibh
#define INCbucketLibh

#ifdef __cplusplus
extern "C" {
#endif

#include "errMdef.h"
#include "epicsTypes.h"
#include "libComAPI.h"

typedef	unsigned 	BUCKETID;

typedef enum {bidtUnsigned, bidtPointer, bidtString} buckTypeOfId;

typedef struct item{
	struct item	*pItem;
	const void	*pId;
	const void   *pApp;
	buckTypeOfId	type;
}ITEM;

typedef struct bucket{
	ITEM		**pTable;
	void		*freeListPVT;
	unsigned	hashIdMask;
	unsigned	hashIdNBits;
        unsigned        nInUse;
}BUCKET;

LIBCOM_API BUCKET * LIBCOMSTD_API bucketCreate (unsigned nHashTableEntries);
LIBCOM_API int LIBCOMSTD_API bucketFree (BUCKET *prb);
LIBCOM_API int LIBCOMSTD_API bucketShow (BUCKET *pb);

/*
 * !! Identifier must exist (and remain constant) at the specified address until
 * the item is deleted from the bucket !!
 */
LIBCOM_API int LIBCOMSTD_API bucketAddItemUnsignedId (BUCKET *prb, 
		const unsigned *pId, const void *pApp);
LIBCOM_API int LIBCOMSTD_API bucketAddItemPointerId (BUCKET *prb, 
		void * const *pId, const void *pApp);
LIBCOM_API int LIBCOMSTD_API bucketAddItemStringId (BUCKET *prb, 
		const char *pId, const void *pApp);

LIBCOM_API int LIBCOMSTD_API bucketRemoveItemUnsignedId (BUCKET *prb, const unsigned *pId);
LIBCOM_API int LIBCOMSTD_API bucketRemoveItemPointerId (BUCKET *prb, void * const *pId);
LIBCOM_API int LIBCOMSTD_API bucketRemoveItemStringId (BUCKET *prb, const char *pId);

LIBCOM_API void * LIBCOMSTD_API bucketLookupItemUnsignedId (BUCKET *prb, const unsigned *pId);
LIBCOM_API void * LIBCOMSTD_API bucketLookupItemPointerId (BUCKET *prb, void * const *pId);
LIBCOM_API void * LIBCOMSTD_API bucketLookupItemStringId (BUCKET *prb, const char *pId);

LIBCOM_API void * LIBCOMSTD_API bucketLookupAndRemoveItemUnsignedId (BUCKET *prb, const unsigned *pId);
LIBCOM_API void * LIBCOMSTD_API bucketLookupAndRemoveItemPointerId (BUCKET *prb, void * const *pId);
LIBCOM_API void * LIBCOMSTD_API bucketLookupAndRemoveItemStringId (BUCKET *prb, const char *pId);


/*
 * Status returned by bucketLib functions
 */
#define BUCKET_SUCCESS		S_bucket_success
#define S_bucket_success	0
#define S_bucket_noMemory	(M_bucket | 1) 	/*Memory allocation failed*/
#define S_bucket_idInUse	(M_bucket | 2) 	/*Identifier already in use*/
#define S_bucket_uknId		(M_bucket | 3) 	/*Unknown identifier*/

#ifdef __cplusplus
}
#endif

#endif /*INCbucketLibh*/

