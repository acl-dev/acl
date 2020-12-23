#include "StdAfx.h"
#include <stdio.h>
#ifndef ACL_PREPARE_COMPILE
#include "stdlib/acl_define.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "json/acl_json.h"
#include "stdlib/acl_vstring.h"
#endif

#define	LEN	ACL_VSTRING_LEN
#define	STR	acl_vstring_str

static ACL_JSON_NODE *node_iter_head(ACL_ITER *it, ACL_JSON_NODE *node)
{
	ACL_RING *ring_ptr;

	it->dlen = -1;
	it->key = NULL;
	it->klen = -1;

	it->i = 0;
	it->size = node->children.len;

	if ((ring_ptr = acl_ring_succ(&node->children)) == &node->children) {
		it->ptr = it->data = NULL;
		return NULL;
	}
	it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
	it->data = it->ptr;
	return it->ptr;
}

static ACL_JSON_NODE *node_iter_next(ACL_ITER *it, ACL_JSON_NODE *node)
{
	ACL_RING *ring_ptr;
	struct ACL_JSON_NODE *child;

	child = (struct ACL_JSON_NODE*) it->data;
	if ((ring_ptr = acl_ring_succ(&child->node)) == &node->children) {
		it->ptr = it->data = NULL;
		return NULL;
	}
	it->i++;
	it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
	it->data = it->ptr;
	return it->ptr;
}

static ACL_JSON_NODE *node_iter_tail(ACL_ITER *it, ACL_JSON_NODE *node)
{
	ACL_RING *ring_ptr;

	it->dlen = -1;
	it->key = NULL;
	it->klen = -1;

	it->i = 0;
	it->size = node->children.len;

	if ((ring_ptr = acl_ring_pred(&node->children)) == &node->children) {
		it->ptr = it->data = NULL;
		return NULL;
	}

	it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
	it->data = it->ptr;
	return it->ptr;
}

static ACL_JSON_NODE *node_iter_prev(ACL_ITER *it, ACL_JSON_NODE *node)
{
	ACL_RING *ring_ptr;
	struct ACL_JSON_NODE *child;

	child = (struct ACL_JSON_NODE*) it->data;
	if ((ring_ptr = acl_ring_pred(&child->node)) == &node->children) {
		it->ptr = it->data = NULL;
		return NULL;
	}
	it->i++;
	it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
	it->data = it->ptr;
	return it->ptr;
}

ACL_JSON_NODE *acl_json_node_alloc(ACL_JSON *json)
{
	ACL_JSON_NODE *node;

	node = (ACL_JSON_NODE*) acl_dbuf_pool_alloc(
			json->dbuf, sizeof(ACL_JSON_NODE));

	acl_ring_init(&node->children);
	acl_ring_init(&node->node);

	node->ltag      = acl_vstring_dbuf_alloc(json->dbuf, 16);
	node->text      = acl_vstring_dbuf_alloc(json->dbuf, 16);
	node->tag_node  = NULL;
	node->parent    = NULL;
	node->type      = 0;
	node->depth     = 0;
	node->quote     = 0;
	node->left_ch   = 0;
	node->right_ch  = 0;
	node->backslash = 0;
	node->part_word = 0;
	node->json      = json;

	node->iter_head = node_iter_head;
	node->iter_next = node_iter_next;
	node->iter_tail = node_iter_tail;
	node->iter_prev = node_iter_prev;

	json->node_cnt++;
	return node;
}

int acl_json_node_delete(ACL_JSON_NODE *node)
{
	ACL_RING *next;
	ACL_JSON_NODE *node_next;
	int   n = 1;

	while ((next = acl_ring_pop_head(&node->children)) != NULL) {
		node_next = acl_ring_to_appl(next, ACL_JSON_NODE, node);
		n += acl_json_node_delete(node_next);
	}

	acl_ring_detach(&node->node);
	node->json->node_cnt--;

	return n;
}

void acl_json_node_append(ACL_JSON_NODE *node1, ACL_JSON_NODE *node2)
{
	acl_ring_append(&node1->node, &node2->node);
	node2->parent = node1->parent;
}

void acl_json_node_add_child(ACL_JSON_NODE *parent, ACL_JSON_NODE *child)
{
	acl_ring_prepend(&parent->children, &child->node);
	child->parent = parent;
}

ACL_JSON_NODE *acl_json_node_parent(ACL_JSON_NODE *node)
{
	return node->parent;
}

ACL_JSON_NODE *acl_json_node_next(ACL_JSON_NODE *node)
{
	ACL_RING *ring_ptr = acl_ring_succ(&node->node);
	ACL_JSON_NODE *parent;

	if (ring_ptr == &node->node)
		return NULL;
	parent = node->parent;
	acl_assert(parent != NULL);
	if (ring_ptr == &parent->children)
		return NULL;
	return acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
}

ACL_JSON_NODE *acl_json_node_prev(ACL_JSON_NODE *node)
{
	ACL_RING *ring_ptr = acl_ring_pred(&node->node);
	ACL_JSON_NODE *parent;

	if (ring_ptr == &node->node)
		return NULL;
	parent = node->parent;
	acl_assert(parent != NULL);
	if (ring_ptr == &parent->children)
		return NULL;

	return acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
}

/************************************************************************/
/*               json 对象处理函数集                                    */
/************************************************************************/

static ACL_JSON_NODE *json_iter_head(ACL_ITER *it, ACL_JSON *json)
{
	ACL_RING *ring_ptr;

	it->dlen = -1;
	it->key = NULL;
	it->klen = -1;

	it->i = 0;
	it->size = json->node_cnt;

	ring_ptr = acl_ring_succ(&json->root->children);
	if (ring_ptr== &json->root->children) {
		it->ptr = it->data = NULL;
		return NULL;
	}
	it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
	it->data = it->ptr;

	return it->ptr;
}

static ACL_JSON_NODE *json_iter_next(ACL_ITER *it, ACL_JSON *json)
{
	ACL_RING *ring_ptr;
	struct ACL_JSON_NODE *node, *parent;

	node = (struct ACL_JSON_NODE*) it->data;

	/* 先遍历当前节点的子节点 */

	ring_ptr = acl_ring_succ(&node->children);
	if (ring_ptr != &node->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
		it->data = it->ptr;
		return it->ptr;
	}

	/* 当前节点的子节点遍历完毕，再遍历当前节点的兄弟节点 */

	parent = acl_json_node_parent(node);
	ring_ptr = acl_ring_succ(&node->node);
	if (ring_ptr != &parent->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
		it->data = it->ptr;
		return it->ptr;
	}

	/* 当前节点的兄弟节点遍历完毕，最后遍历当前节点的父节点的兄弟节点 */

	do {
		if (parent == json->root)
			break;

		ring_ptr = acl_ring_succ(&parent->node);
		parent = acl_json_node_parent(parent);
		if (parent == NULL)
			acl_msg_fatal("%s(%d): parent null", __FILE__, __LINE__);

		if (ring_ptr != &parent->children) {
			it->i++;
			it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
			it->data = it->ptr;
			return it->ptr;
		}
	} while (ring_ptr != &json->root->children);

	/* 遍历完所有节点 */

	it->ptr = it->data = NULL;
	return NULL;
}

static ACL_JSON_NODE *json_iter_tail(ACL_ITER *it, ACL_JSON *json)
{
	ACL_RING *ring_ptr;

	it->dlen = -1;
	it->key = NULL;
	it->klen = -1;

	it->i = 0;
	it->size = json->node_cnt;

	ring_ptr = acl_ring_pred(&json->root->children);
	if (ring_ptr== &json->root->children) {
		it->ptr = it->data = NULL;
		return NULL;
	}
	it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
	it->data = it->ptr;
	return it->ptr;
}

static ACL_JSON_NODE *json_iter_prev(ACL_ITER *it, ACL_JSON *json)
{
	ACL_RING *ring_ptr;
	struct ACL_JSON_NODE *node, *parent;

	node = (struct ACL_JSON_NODE*) it->data;

	/* 先遍历当前节点的子节点 */

	ring_ptr = acl_ring_pred(&node->children);
	if (ring_ptr != &node->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
		it->data = it->ptr;
		return it->ptr;
	}

	/* 当前节点的子节点遍历完毕，再遍历当前节点的兄弟节点 */

	parent = acl_json_node_parent(node);
	ring_ptr = acl_ring_pred(&node->node);
	if (ring_ptr != &parent->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
		it->data = it->ptr;
		return it->ptr;
	}

	/* 当前节点的兄弟节点遍历完毕，最后遍历当前节点的父节点的兄弟节点 */

	do {
		if (parent == json->root)
			break;
		ring_ptr = acl_ring_pred(&parent->node);
		parent = acl_json_node_parent(parent);
		if (parent == NULL)
			acl_msg_fatal("%s(%d): parent null", __FILE__, __LINE__);

		if (ring_ptr != &parent->children) {
			it->i++;
			it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
			it->data = it->ptr;
			return it->ptr;
		}
	} while (ring_ptr != &json->root->children);

	/* 遍历完所有节点 */

	it->ptr = it->data = NULL;
	return NULL;
}

ACL_JSON *acl_json_alloc()
{
	return acl_json_dbuf_alloc(NULL);
}

ACL_JSON *acl_json_dbuf_alloc(ACL_DBUF_POOL *dbuf)
{
	ACL_JSON *json;

	if (dbuf == NULL) {
		dbuf = acl_dbuf_pool_create(8192);
		json = (ACL_JSON*) acl_dbuf_pool_calloc(dbuf, sizeof(ACL_JSON));
		json->dbuf_inner = dbuf;
	} else {
		json = (ACL_JSON*) acl_dbuf_pool_calloc(dbuf, sizeof(ACL_JSON));
		json->dbuf_inner = NULL;
	}

	json->dbuf = dbuf;
	json->dbuf_keep = sizeof(ACL_JSON);

	json->root = acl_json_node_alloc(json);
	/* 将根节点作为当前节点 */
	json->curr_node = json->root;
	/* 设置状态机的状态 */
#if 0
	json->status = ACL_JSON_S_OBJ;
#else
	json->root->left_ch = '{';
	json->root->right_ch = '}';
	json->status = ACL_JSON_S_ROOT;
#endif

	/* 设置迭代函数 */

	json->iter_head = json_iter_head;
	json->iter_next = json_iter_next;
	json->iter_tail = json_iter_tail;
	json->iter_prev = json_iter_prev;

	return json;
}

ACL_JSON_NODE *acl_json_node_duplicate(ACL_JSON *json, ACL_JSON_NODE *from)
{
	ACL_JSON_NODE *child_from, *child_to, *to;
	ACL_RING_ITER iter;

	to = acl_json_node_alloc(json);
	to->left_ch = from->left_ch;
	to->right_ch = from->right_ch;
	to->type = from->type;
	to->depth = from->depth;  /* XXX? */
	acl_vstring_strcpy(to->ltag, STR(from->ltag));
	acl_vstring_strcpy(to->text, STR(from->text));

	acl_ring_foreach(iter, &from->children) {
		child_from = acl_ring_to_appl(iter.ptr, ACL_JSON_NODE, node);
		child_to = acl_json_node_duplicate(json, child_from);
		acl_json_node_add_child(to, child_to);
		if (from->tag_node == child_from)
			to->tag_node = child_to;
	}

	return to;
}

ACL_JSON *acl_json_create(ACL_JSON_NODE *node)
{
    ACL_DBUF_POOL *dbuf = acl_dbuf_pool_create(8192);
    ACL_JSON *json = acl_json_dbuf_create(dbuf, node);

    json->dbuf_inner = dbuf;
    return json;
}

ACL_JSON *acl_json_dbuf_create(ACL_DBUF_POOL *dbuf, ACL_JSON_NODE *node)
{
	ACL_JSON *json;
	ACL_JSON_NODE *root = node->json->root;

	json = (ACL_JSON*) acl_dbuf_pool_calloc(dbuf, sizeof(ACL_JSON));
	json->dbuf = dbuf;
    json->dbuf_inner = NULL;

	/* 如果传入的节点为 root 节点，则直接赋值创建 root 即可 */
	if (node == root) {
		json->root = acl_json_node_duplicate(json, node);
	} else {
		ACL_JSON_NODE *first;

		json->root = acl_json_node_alloc(json);
		first = acl_json_node_duplicate(json, node);
		acl_json_node_add_child(json->root, first);
	}

	/* 将根节点作为当前节点 */
	json->curr_node = json->root;
	/* 设置状态机的状态 */
#if 0
	json->status = ACL_JSON_S_OBJ;
#else
	json->status = ACL_JSON_S_ROOT;
	json->root->left_ch = '{';
	json->root->right_ch = '}';
#endif

	/* 设置迭代函数 */

	json->iter_head = json_iter_head;
	json->iter_next = json_iter_next;
	json->iter_tail = json_iter_tail;
	json->iter_prev = json_iter_prev;

	return json;
}

void acl_json_foreach_init(ACL_JSON *json, ACL_JSON_NODE *node)
{
	json->root = node;
	json->iter_head = json_iter_head;
	json->iter_next = json_iter_next;
	json->iter_tail = json_iter_tail;
	json->iter_prev = json_iter_prev;
}

void acl_json_free(ACL_JSON *json)
{
	if (json->dbuf_inner)
		acl_dbuf_pool_destroy(json->dbuf_inner);
}

void acl_json_reset(ACL_JSON *json)
{
	if (json->dbuf_inner != NULL)
		acl_dbuf_pool_reset(json->dbuf, json->dbuf_keep);

	json->root = acl_json_node_alloc(json);
	json->root->left_ch = '{';
	json->root->right_ch = '}';
	json->root->type = ACL_JSON_T_OBJ;

	json->node_cnt = 1;
	json->curr_node = json->root;
	json->status = ACL_JSON_S_ROOT;
	json->finish = 0;
	json->depth = 0;
}
