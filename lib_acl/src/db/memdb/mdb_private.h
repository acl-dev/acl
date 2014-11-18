#ifndef	__MDB_PRIVATE_INCLUDE_H__
#define	__MDB_PRIVATE_INCLUDE_H__

#include "struct.h"

ACL_MDT *acl_mdt_hash_create(void);
ACL_MDT *acl_mdt_binhash_create(void);
ACL_MDT *acl_mdt_avl_create(void);

#endif
