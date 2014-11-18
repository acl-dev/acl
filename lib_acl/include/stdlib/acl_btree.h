#ifndef ACL_BTREE_INCLUDE_H
#define ACL_BTREE_INCLUDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "acl_define.h"

/**
 * 二叉树结构类型定义
 */
typedef struct ACL_BTREE ACL_BTREE;

/**
 * 创建一个二叉树对象
 * @return {ACL_BTREE*} 新创建的二叉树对象
 */
ACL_API ACL_BTREE *acl_btree_create(void);

/**
 * 释放一个二叉树对象
 * @param tree {ACL_BTREE*} 二叉树对象
 * @return {int} 0: 成功; -1: 失败
 */
ACL_API int acl_btree_destroy(ACL_BTREE *tree);

/**
 * 从二叉树中查询
 * @param tree {ACL_BTREE*} 二叉树对象
 * @param key {unsigned int} 查询键
 * @return {void*} 查询结果
 */
ACL_API void *acl_btree_find(ACL_BTREE *tree, unsigned int key);

/**
 * 向二叉树中添加
 * @param tree {ACL_BTREE*} 二叉树对象
 * @param key {unsigned int} 键
 * @param data {void*} 动态对象
 */
ACL_API int acl_btree_add(ACL_BTREE *tree, unsigned int key, void *data);

/**
 * 从二叉树中删除
 * @param tree {ACL_BTREE*} 二叉树对象
 * @param key {unsigned int} 键
 * @return {void*} 被删除的动态对象地址, 如果不存在则返回NULL
 */
ACL_API void *acl_btree_remove(ACL_BTREE *tree, unsigned int key);

/**
 * 返回二叉树中最小的键
 * @param tree {ACL_BTREE*} 二叉树对象
 * @param key {unsigned int*} 键指针，存储结果，不能为空
 * @return {int} 0: 表示找到最小键; -1: 表示出错或未找到最小键
 */
ACL_API int acl_btree_get_min_key(ACL_BTREE *tree, unsigned int *key);

/**
 * 返回二叉树中最大的键
 * @param tree {ACL_BTREE*} 二叉树对象
 * @param key {unsigned int*} 键指针，存储结果，不能为空
 * @return {int} 0: 表示找到最大键; -1: 表示出错或未找到最大键
 */
ACL_API int acl_btree_get_max_key(ACL_BTREE *tree, unsigned int *key);

/**
 * 由给定键，返回其在二叉树中的下一个邻近键
 * @param tree {ACL_BTREE*} 二叉树对象
 * @param cur_key {unsigned int} 当前给定键
 * @param next_key {unsigned int*} 存储结果键的指针地址
 * @return {int} 0: 表示找到; -1: 表示出错或未找到
 */
ACL_API int acl_btree_get_next_key(ACL_BTREE *tree,
	unsigned int cur_key, unsigned int *next_key);

/**
 * 计算当前二叉树的深度
 * @param tree {ACL_BTREE*} 二叉树对象
 * @return {int} 二叉树的深度
 */
ACL_API int acl_btree_depth(ACL_BTREE *tree);
ACL_API void acl_btree_dump(ACL_BTREE *b);

#ifdef __cplusplus
}
#endif

#endif

