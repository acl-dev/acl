#ifndef ACL_DLINK_INCLUDE
#define ACL_DLINK_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_array.h"
#include "acl_iterator.h"

/**
 * Block range element type definition.
 */
typedef	struct {
	acl_int64 begin;
	acl_int64 end;
	void *pnode;
} ACL_DITEM;

/**
 * Block range container type definition.
 */
typedef	struct ACL_DLINK {
	ACL_ARRAY *parray;
	void *call_back_data;

	/* for acl_iterator */

	/* Get iterator head pointer */
	void *(*iter_head)(ACL_ITER*, struct ACL_DLINK*);
	/* Get next iterator pointer */
	void *(*iter_next)(ACL_ITER*, struct ACL_DLINK*);
	/* Get iterator tail pointer */
	void *(*iter_tail)(ACL_ITER*, struct ACL_DLINK*);
	/* Get previous iterator pointer */
	void *(*iter_prev)(ACL_ITER*, struct ACL_DLINK*);
} ACL_DLINK;

/**
 * Create a block range container object.
 * @param nsize {int} Initial array size
 * @return {ACL_DLINK*} Block range container object
 */
ACL_API ACL_DLINK *acl_dlink_create(int nsize);

/**
 * Free a block range container object.
 * @param plink {ACL_DLINK*} Block range container object pointer
 */
ACL_API void acl_dlink_free(ACL_DLINK *plink);

/**
 * Check whether block range element exists in block range container based on block range element.
 * @param plink {ACL_DLINK*} Block range container object pointer
 * @param pitem {ACL_DITEM*} Block element
 * @return {ACL_DITEM*} Block element
 */
ACL_API ACL_DITEM *acl_dlink_lookup_by_item(const ACL_DLINK *plink,
	ACL_DITEM *pitem);

/**
 * Search for block element in block range container based on block element.
 * @param plink {const ACL_DLINK*} Block range container object pointer
 * @param pitem {ACL_DITEM*} Block element
 * @param pidx {int*} Storage for index position of queried element result in container array
 * @return {ACL_DITEM*} Block element
 */
ACL_API ACL_DITEM *acl_dlink_lookup2_by_item(const ACL_DLINK *plink,
	ACL_DITEM *pitem, int *pidx);

/**
 * Query address of block element corresponding to a certain value from block range container.
 * @param plink {const ACL_DLINK*} Block range container object pointer
 * @param n {acl_int64} Query value
 * @return {ACL_DITEM*} Block element
 */
ACL_API ACL_DITEM *acl_dlink_lookup(const ACL_DLINK *plink, acl_int64 n);

/**
 * Query address of block element corresponding to a certain value from block range container and record index position.
 * @param plink {const ACL_DLINK*} Block range container object pointer
 * @param n {acl_int64} Query value
 * @param pidx {int*} Storage for index position of queried element result in container array
 * @return {ACL_DITEM*} Block element
 */
ACL_API ACL_DITEM *acl_dlink_lookup2(const ACL_DLINK *plink,
	acl_int64 n, int *pidx);

/**
 * Query address of block element corresponding to a certain range from block range container and record index position.
 * @param plink {const ACL_DLINK*} Block range container object pointer
 * @param begin {acl_int64} Query range start position value
 * @param end {acl_int64} Query range end position value
 * @param pidx {int*} Storage for index position of queried element result in container array
 * @return {ACL_DITEM*} Block element
 */
ACL_API ACL_DITEM *acl_dlink_lookup_range(const ACL_DLINK *plink,
	acl_int64 begin, acl_int64 end, int *pidx);

/**
 * Query block element larger than a certain value from block range container and record index position.
 * @param plink {const ACL_DLINK*} Block range container object pointer
 * @param off {acl_int64} Comparison value
 * @param pidx {int*} Storage for index position of queried element result in container array
 * @return {ACL_DITEM*} Block element
 */
ACL_API ACL_DITEM *acl_dlink_lookup_larger(const ACL_DLINK *plink,
	acl_int64 off, int *pidx);

/**
 * Query block element smaller than a certain value from block range container and record index position.
 * @param plink {const ACL_DLINK*} Block range container object pointer
 * @param off {acl_int64} Comparison value
 * @param pidx {int*} Storage for index position of queried element result in container array
 * @return {ACL_DITEM*} Block element
 */
ACL_API ACL_DITEM *acl_dlink_lookup_lower(const ACL_DLINK *plink,
	acl_int64 off, int *pidx);

/**
 * Insert block range into container starting from specified position to create block.
 * @param plink {ACL_DLINK*} Block range container object pointer
 * @param begin {acl_int64} Block start position value
 * @param end {acl_int64} Block end position value
 * @return {ACL_DITEM*} Newly created block element
 */
ACL_API ACL_DITEM *acl_dlink_insert(ACL_DLINK *plink,
	acl_int64 begin, acl_int64 end);

/**
 * Delete block element corresponding to a certain position value from block range container.
 * @param plink {ACL_DLINK*} Block range container object pointer
 * @param n {acl_int64} Position value
 * @return {int} 0 indicates OK; -1: indicates error or block does not exist
 */
ACL_API int acl_dlink_delete(ACL_DLINK *plink, acl_int64 n);

/**
 * Delete block element from block range container based on block element.
 * @param plink {ACL_DLINK*} Block range container object pointer
 * @param pitem {ACL_DITEM*} Block element
 * @return {int} 0 indicates OK; -1: indicates error
 */
ACL_API int acl_dlink_delete_by_item(ACL_DLINK *plink, ACL_DITEM *pitem);

/**
 * Same as acl_dlink_insert
 * @DEPRECATED This function may not be provided in the future
 */
ACL_API ACL_DITEM *acl_dlink_modify(ACL_DLINK *plink,
	acl_int64 begin, acl_int64 end);

/**
 * Delete block set for a certain value range from block range container,
 * deletion may internally create new block elements.
 * @param plink {ACL_DLINK*} Block range container object pointer
 * @param begin {acl_int64} Start position of range to delete
 * @param end {acl_int64} End position of range to delete
 * @return {int} 0 indicates OK; -1: indicates error
 */
ACL_API int acl_dlink_delete_range(ACL_DLINK *plink,
	acl_int64 begin, acl_int64 end);

/**
 * Get address of block element at a certain index position.
 * @param plink {const ACL_DLINK*} Block range container object pointer
 * @param idx {int} Index position
 * @return {ACL_DITEM*} NULL: index out of bounds; != NULL: block element address
 */
ACL_API ACL_DITEM *acl_dlink_index(const ACL_DLINK *plink, int idx);

/**
 * Get total number of blocks in current container.
 * @param plink {const ACL_DLINK*} Block range container object pointer
 * @return {int} Number of blocks
 */
ACL_API int acl_dlink_size(const ACL_DLINK *plink);

/**
 * (Debug) Print information about start and end positions of all blocks in container.
 * @param plink {const ACL_DLINK*} Block range container object pointer
 * @return {int} 0 indicates OK; -1: indicates error
 */
ACL_API int acl_dlink_list(const ACL_DLINK *plink);

#ifdef __cplusplus
}
#endif
#endif
