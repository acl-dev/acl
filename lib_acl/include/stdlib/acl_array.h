#ifndef	ACL_ARRAY_INCLUDE_H
#define	ACL_ARRAY_INCLUDE_H

#ifdef  __cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_dbuf_pool.h"
#include "acl_iterator.h"

/**
 * Dynamic array type definition.
 */
typedef	struct ACL_ARRAY ACL_ARRAY;
struct ACL_ARRAY{
	ACL_DBUF_POOL *dbuf;	/**< Memory pool object */
	int     capacity;	/**< items array space size */
	int     count;		/**< number of valid elements in items */
	void    **items;	/**< Dynamic array */

	/* Operation methods */

	/* Append dynamic object to array tail */
	void  (*push_back)(struct ACL_ARRAY*, void*);
	/* Prepend dynamic object to array head */
	void  (*push_front)(struct ACL_ARRAY*, void*);
	/* Pop dynamic object from array tail */
	void *(*pop_back)(struct ACL_ARRAY*);
	/* Pop dynamic object from array head */
	void *(*pop_front)(struct ACL_ARRAY*);

	/* for acl_iterator */

	/* Get iterator head pointer */
	void *(*iter_head)(ACL_ITER*, struct ACL_ARRAY*);
	/* Get next iterator pointer */
	void *(*iter_next)(ACL_ITER*, struct ACL_ARRAY*);
	/* Get iterator tail pointer */
	void *(*iter_tail)(ACL_ITER*, struct ACL_ARRAY*);
	/* Get previous iterator pointer */
	void *(*iter_prev)(ACL_ITER*, struct ACL_ARRAY*);
};

/**
 * Create a dynamic array.
 * @param init_size {int} Initial size of dynamic array
 * @return {ACL_ARRAY*} Dynamic array pointer
 */
ACL_API ACL_ARRAY *acl_array_create(int init_size);

/**
 * Create a dynamic array.
 * @param init_size {int} Initial size of dynamic array
 * @param dbuf {ACL_DBUF_POOL*} If not NULL, use memory pool
 *  (dynamically allocated) for allocation
 * @return {ACL_ARRAY*} Dynamic array pointer
 */
ACL_API ACL_ARRAY *acl_array_dbuf_create(int init_size, ACL_DBUF_POOL *dbuf);

/**
 * Free members in dynamic array, but do not free dynamic array object.
 * @param a {ACL_ARRAY*} Dynamic array pointer
 * @param free_fn {void (*)(void*)} Callback function to free
 *  dynamic objects in members, free function pointer
 */
ACL_API void acl_array_clean(ACL_ARRAY *a, void (*free_fn)(void *));

/**
 * Free members in dynamic array and free dynamic array
 * object. If object was created with dbuf parameter, then
 * members will not be freed separately, they will be freed
 * when dbuf is freed.
 * @param a {ACL_ARRAY*} Dynamic array pointer
 * @param free_fn {void (*)(void*)} Callback function to free
 *  dynamic objects in members, free function pointer
 */
ACL_API void acl_array_free(ACL_ARRAY *a, void (*free_fn)(void *));
#define acl_array_destroy acl_array_free

/**
 * Append dynamic member object to array tail.
 * @param a {ACL_ARRAY*} Dynamic array pointer
 * @param obj {void*} Dynamic member object
 * @return {int} >=0: success, return value is element's
 *  index position in array; -1: failure
 */
ACL_API int acl_array_append(ACL_ARRAY *a, void *obj);

/**
 * Prepend dynamic member object to array head.
 * @param a {ACL_ARRAY*} Dynamic array pointer
 * @param obj {void*} Dynamic member object
 * @return {int} >=0: success, return value is element's
 *  index position in array; -1: failure
 */
ACL_API int acl_array_prepend(ACL_ARRAY *a, void *obj);

/**
 * Insert dynamic member object before specified position in
 * array (this node and all subsequent nodes move one
 * position).
 * @param a {ACL_ARRAY*} Dynamic array pointer
 * @param position {int} A certain position, must not exceed bounds
 * @param obj {void*} Dynamic member object
 * @return {int} 0: success; -1: failure
 */
ACL_API int acl_array_pred_insert(ACL_ARRAY *a, int position, void *obj);

/**
 * Insert dynamic member object after specified position in
 * array (this node and all subsequent nodes move one
 * position).
 * @param a {ACL_ARRAY*} Dynamic array pointer
 * @param position {int} A certain position, must not exceed bounds
 * @param obj {void*} Dynamic member object
 * @return {int} 0: success; -1: failure
 */
ACL_API int acl_array_succ_insert(ACL_ARRAY *a, int position, void *obj);
#define acl_array_insert acl_array_succ_insert

/**
 * Delete a dynamic object from specified position in
 * dynamic array. After deletion, order of remaining elements
 * remains unchanged. If deletion position is in the middle,
 * to ensure element order, internally after deleting
 * element, subsequent elements move forward one position.
 * @param a {ACL_ARRAY*} Dynamic array pointer
 * @param position {int} A certain position, must not exceed bounds
 * @param free_fn {void (*)(void*)} Callback function to free
 *  dynamic objects in members, free function pointer. If
 *  pointer is NULL, do not free, otherwise use this function to free dynamic object
 * @return {int} 0: success; -1: failure
 */
ACL_API int acl_array_delete_idx(ACL_ARRAY *a, int position, void (*free_fn)(void *));

/**
 * Delete a dynamic object from specified position in
 * dynamic array. After deletion, order of remaining elements
 * may change, because deletion moves the last element to
 * this position.
 * @param a {ACL_ARRAY*} Dynamic array pointer
 * @param position {int} A certain position, must not exceed bounds
 * @param free_fn {void (*)(void*)} Callback function to free
 *  dynamic objects in members, free function pointer. If
 *  pointer is NULL, do not free, otherwise use this function to free dynamic object
 * @return {int} 0: success; -1: failure
 */
ACL_API int acl_array_delete(ACL_ARRAY *a, int position, void (*free_fn)(void*));

/**
 * Delete dynamic object with specified pointer address from
 * dynamic array. After deletion, order of remaining elements
 * remains unchanged. If deletion position is in the middle,
 * to ensure element order, internally after deleting
 * element, subsequent elements move forward one position.
 * @param a {ACL_ARRAY*} Dynamic array pointer
 * @param obj {void*} Dynamic object pointer address
 * @param free_fn {void (*)(void*)} Callback function to free
 *  dynamic objects in members, free function pointer. If
 *  pointer is NULL, do not free, otherwise use this function to free dynamic object
 * @return {int} 0: success; -1: failure
 */
ACL_API int acl_array_delete_obj(ACL_ARRAY *a, void *obj, void (*free_fn)(void *));

/**
 * Delete dynamic objects in a certain index range from dynamic array.
 * @param a {ACL_ARRAY*} Dynamic array pointer
 * @param ibegin {int} Start index position
 * @param iend {int} End index position
 * @param free_fn {void (*)(void*)} Callback function to free
 *  dynamic objects in members, free function pointer. If
 *  pointer is NULL, do not free, otherwise use this function to free dynamic object
 * @return {int} 0: success; -1: failure
 */
ACL_API int acl_array_delete_range(ACL_ARRAY *a, int ibegin, int iend, void (*free_fn)(void*));

/**
 * Move objects in dynamic array.
 * @param a {ACL_ARRAY*} Dynamic array pointer
 * @param ito {int} Target index position to move to
 * @param ifrom {int} Start index position to move from
 * @param free_fn {void (*)(void*)} Callback function to free
 *  dynamic objects in members, free function pointer. If
 *  pointer is NULL, do not free, otherwise use this
 *  function to free dynamic object. Freed dynamic objects
 *  are [idx_obj_begin, idx_src_begin), a half-open
 *  interval
 * @return {int} 0: success; -1: failure
 */
ACL_API int acl_array_mv_idx(ACL_ARRAY *a, int ito, int ifrom, void (*free_fn)(void *) );

/**
 * Pre-ensure dynamic array's space length.
 * @param a {ACL_ARRAY*} Dynamic array pointer
 * @param app_count {int} Need to ensure dynamic array has
 *  app_count empty positions
 */
ACL_API void acl_array_pre_append(ACL_ARRAY *a, int app_count);

/**
 * Get dynamic object from a certain index position in dynamic array.
 * @param a {ACL_ARRAY*} Dynamic array pointer
 * @param idx {int} Index position, must not exceed bounds,
 *  otherwise returns -1
 * @return {void*} != NULL: success; == NULL: does not exist or failure
 */
ACL_API void *acl_array_index(const ACL_ARRAY *a, int idx);

/**
 * Get current number of dynamic objects in dynamic array.
 * @param a {ACL_ARRAY*} Dynamic array pointer
 * @return {int} Number of dynamic objects in dynamic array
 */
ACL_API int acl_array_size(const ACL_ARRAY *a);

#ifdef  __cplusplus
}
#endif

#endif
