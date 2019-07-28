/*
 * Name: iplink.h
 * Author: zsx
 * Date: 2003/11/30
 * Version: 1.0
 *
*/
#ifndef	ACL_IPLINK_INCLUDE_H
#define	ACL_IPLINK_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_dlink.h"
#include "acl_iterator.h"

#define	ACL_IPITEM ACL_DITEM
#define	ACL_IPLINK ACL_DLINK

ACL_API ACL_IPLINK *acl_iplink_create(int nsize);
ACL_API void acl_iplink_free(ACL_IPLINK *plink);
ACL_API ACL_IPITEM *acl_iplink_lookup_item(const ACL_IPLINK *plink,
	ACL_IPITEM *pitem);
ACL_API ACL_IPITEM *acl_iplink_lookup_bin(const ACL_IPLINK *plink,
	unsigned int ip);
ACL_API ACL_IPITEM *acl_iplink_lookup_str(const ACL_IPLINK *plink,
	const char *ip);
ACL_API ACL_IPITEM *acl_iplink_insert_bin(ACL_IPLINK *plink,
	unsigned int ip_begin, unsigned int ip_end);
ACL_API ACL_IPITEM *acl_iplink_insert(ACL_IPLINK *plink,
	const char *pstrip_begin, const char *pstrip_end);
ACL_API int acl_iplink_delete_by_ip(ACL_IPLINK *plink,
	const char *pstrip_begin);
ACL_API int acl_iplink_delete_by_item(ACL_IPLINK *plink, ACL_IPITEM *pitem);
ACL_API ACL_IPITEM *acl_iplink_modify(ACL_IPLINK *plink, const char *pstrip_id,
	const char *pstrip_begin, const char *pstrip_end);
ACL_API int acl_iplink_count_item(ACL_IPLINK *plink);
ACL_API int acl_iplink_list(const ACL_IPLINK *plink);

#ifdef  __cplusplus
}
#endif

#endif

