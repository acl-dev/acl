#ifndef ACL_BTREE_INCLUDE_H
#define ACL_BTREE_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * Binary tree structure type definition.
 */
typedef struct ACL_BTREE ACL_BTREE;

/**
 * Create a binary tree object.
 * @return {ACL_BTREE*} Newly created binary tree object
 */
ACL_API ACL_BTREE *acl_btree_create(void);

/**
 * Free a binary tree object.
 * @param tree {ACL_BTREE*} Binary tree object
 * @return {int} 0: success; -1: failure
 */
ACL_API int acl_btree_destroy(ACL_BTREE *tree);

/**
 * Search in binary tree.
 * @param tree {ACL_BTREE*} Binary tree object
 * @param key {unsigned int} Search key
 * @return {void*} Search result
 */
ACL_API void *acl_btree_find(ACL_BTREE *tree, unsigned int key);

/**
 * Add to binary tree.
 * @param tree {ACL_BTREE*} Binary tree object
 * @param key {unsigned int} Key
 * @param data {void*} Dynamic data
 */
ACL_API int acl_btree_add(ACL_BTREE *tree, unsigned int key, void *data);

/**
 * Remove from binary tree.
 * @param tree {ACL_BTREE*} Binary tree object
 * @param key {unsigned int} Key
 * @return {void*} Address of removed dynamic data, if key does not exist, returns NULL
 */
ACL_API void *acl_btree_remove(ACL_BTREE *tree, unsigned int key);

/**
 * Get the minimum key in binary tree.
 * @param tree {ACL_BTREE*} Binary tree object
 * @param key {unsigned int*} Key pointer, stores result, must not be NULL
 * @return {int} 0: indicates found minimum key; -1: indicates tree is empty
 *  or minimum key not found
 */
ACL_API int acl_btree_get_min_key(ACL_BTREE *tree, unsigned int *key);

/**
 * Get the maximum key in binary tree.
 * @param tree {ACL_BTREE*} Binary tree object
 * @param key {unsigned int*} Key pointer, stores result, must not be NULL
 * @return {int} 0: indicates found maximum key; -1: indicates tree is empty
 *  or maximum key not found
 */
ACL_API int acl_btree_get_max_key(ACL_BTREE *tree, unsigned int *key);

/**
 * Get the next node in binary tree from a given key.
 * @param tree {ACL_BTREE*} Binary tree object
 * @param cur_key {unsigned int} Current key value
 * @param next_key {unsigned int*} Storage address pointer for result
 * @return {int} 0: indicates found; -1: indicates tree is empty or not found
 */
ACL_API int acl_btree_get_next_key(ACL_BTREE *tree,
	unsigned int cur_key, unsigned int *next_key);

/**
 * Calculate current binary tree depth.
 * @param tree {ACL_BTREE*} Binary tree object
 * @return {int} Binary tree depth
 */
ACL_API int acl_btree_depth(ACL_BTREE *tree);
ACL_API void acl_btree_dump(ACL_BTREE *b);

#ifdef __cplusplus
}
#endif

#endif
