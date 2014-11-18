#include "StdAfx.h"
#include <stdio.h>
#ifndef ACL_PREPARE_COMPILE
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
		return (NULL);
	}
	it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
	it->data = it->ptr;
	return (it->ptr);
}

static ACL_JSON_NODE *node_iter_next(ACL_ITER *it, ACL_JSON_NODE *node)
{
	ACL_RING *ring_ptr;
	struct ACL_JSON_NODE *child;

	child = (struct ACL_JSON_NODE*) it->data;
	if ((ring_ptr = acl_ring_succ(&child->node)) == &node->children) {
		it->ptr = it->data = NULL;
		return (NULL);
	}
	it->i++;
	it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
	it->data = it->ptr;
	return (it->ptr);
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
		return (NULL);
	}

	it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
	it->data = it->ptr;
	return (it->ptr);
}

static ACL_JSON_NODE *node_iter_prev(ACL_ITER *it, ACL_JSON_NODE *node)
{
	ACL_RING *ring_ptr;
	struct ACL_JSON_NODE *child;

	child = (struct ACL_JSON_NODE*) it->data;
	if ((ring_ptr = acl_ring_pred(&child->node)) == &node->children) {
		it->ptr = it->data = NULL;
		return (NULL);
	}
	it->i++;
	it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
	it->data = it->ptr;
	return (it->ptr);
}

static void acl_json_node_reset(ACL_JSON_NODE *node)
{
	ACL_VSTRING_RESET(node->ltag);
	ACL_VSTRING_RESET(node->text);
	ACL_VSTRING_TERMINATE(node->ltag);
	ACL_VSTRING_TERMINATE(node->text);

	node->tag_node = NULL;
	node->type = ACL_JSON_T_LEAF;

	node->parent = NULL;
	acl_ring_init(&node->children);
	node->depth = 0;

	acl_ring_init(&node->node);
	node->quote = 0;
	node->backslash = 0;
	node->left_ch = node->right_ch = 0;
	node->part_word = 0;
}

ACL_JSON_NODE *acl_json_node_alloc(ACL_JSON *json)
{
	ACL_JSON_NODE *node;

	if (json->node_cache) {
		node = (ACL_JSON_NODE*)
			json->node_cache->pop_back(json->node_cache);
		if (node) {
			acl_json_node_reset(node);
			node->json = json;
			json->node_cnt++;
			return (node);
		}
	}

	if (json->slice)
		node = (ACL_JSON_NODE*) acl_slice_pool_calloc(__FILE__, __LINE__,
				json->slice, 1, sizeof(ACL_JSON_NODE));
	else
		node = (ACL_JSON_NODE*) acl_mycalloc(1, sizeof(ACL_JSON_NODE));
	acl_ring_init(&node->children);
	acl_ring_init(&node->node);

	node->json = json;
	node->ltag = acl_vstring_alloc2(json->slice, 16);
	node->text = acl_vstring_alloc2(json->slice, 16);
	node->part_word = 0;
	json->node_cnt++;

	node->iter_head = node_iter_head;
	node->iter_next = node_iter_next;
	node->iter_tail = node_iter_tail;
	node->iter_prev = node_iter_prev;
	return (node);
}

static void acl_json_node_free(ACL_JSON_NODE *node)
{
	acl_vstring_free(node->ltag);
	acl_vstring_free(node->text);
	acl_ring_detach(&node->node);
	if (node->json->slice)
		acl_slice_pool_free(__FILE__, __LINE__, node);
	else
		acl_myfree(node);
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

	node->json->node_cnt--;
	if (node->json->node_cache &&
		acl_array_size(node->json->node_cache) < node->json->max_cache)
	{
		node->json->node_cache->push_back(node->json->node_cache, node);
		acl_ring_detach(&node->node);
	} else
		acl_json_node_free(node);
	return (n);
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
	return (node->parent);
}

ACL_JSON_NODE *acl_json_node_next(ACL_JSON_NODE *node)
{
	ACL_RING *ring_ptr = acl_ring_succ(&node->node);
	ACL_JSON_NODE *parent;

	if (ring_ptr == &node->node)
		return (NULL);
	parent = node->parent;
	acl_assert(parent != NULL);
	if (ring_ptr == &parent->children)
		return (NULL);
	return (acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node));
}

ACL_JSON_NODE *acl_json_node_prev(ACL_JSON_NODE *node)
{
	ACL_RING *ring_ptr = acl_ring_pred(&node->node);
	ACL_JSON_NODE *parent;

	if (ring_ptr == &node->node)
		return (NULL);
	parent = node->parent;
	acl_assert(parent != NULL);
	if (ring_ptr == &parent->children)
		return (NULL);

	return (acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node));
}

const char *acl_json_node_type(const ACL_JSON_NODE *node)
{
	static char *types_tab[] = { "leaf node", "node object", "array object" };
	static char *unknown = "unknown";

	if (node->type >= 0 && node->type <= 2)
		return (types_tab[node->type]);
	return (unknown);
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
		return (NULL);
	}
	it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
	it->data = it->ptr;

	return (it->ptr);
}

static ACL_JSON_NODE *json_iter_next(ACL_ITER *it, ACL_JSON *json)
{
	ACL_RING *ring_ptr;
	struct ACL_JSON_NODE *node, *parent;

	node = (struct ACL_JSON_NODE*) it->data;

	/* 先遍历当前结点的子结点 */

	ring_ptr = acl_ring_succ(&node->children);
	if (ring_ptr != &node->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
		it->data = it->ptr;
		return (it->ptr);
	}

	/* 当前结点的子结点遍历完毕，再遍历当前结点的兄弟结点 */

	parent = acl_json_node_parent(node);
	ring_ptr = acl_ring_succ(&node->node);
	if (ring_ptr != &parent->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
		it->data = it->ptr;
		return (it->ptr);
	}

	/* 当前结点的兄弟结点遍历完毕，最后遍历当前结点的父结点的兄弟结点 */

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
			return (it->ptr);
		}
	} while (ring_ptr != &json->root->children);

	/* 遍历完所有结点 */

	it->ptr = it->data = NULL;
	return (NULL);
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
		return (NULL);
	}
	it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
	it->data = it->ptr;
	return (it->ptr);
}

static ACL_JSON_NODE *json_iter_prev(ACL_ITER *it, ACL_JSON *json)
{
	ACL_RING *ring_ptr;
	struct ACL_JSON_NODE *node, *parent;

	node = (struct ACL_JSON_NODE*) it->data;

	/* 先遍历当前结点的子结点 */

	ring_ptr = acl_ring_pred(&node->children);
	if (ring_ptr != &node->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
		it->data = it->ptr;
		return (it->ptr);
	}

	/* 当前结点的子结点遍历完毕，再遍历当前结点的兄弟结点 */

	parent = acl_json_node_parent(node);
	ring_ptr = acl_ring_pred(&node->node);
	if (ring_ptr != &parent->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, ACL_JSON_NODE, node);
		it->data = it->ptr;
		return (it->ptr);
	}

	/* 当前结点的兄弟结点遍历完毕，最后遍历当前结点的父结点的兄弟结点 */

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
			return (it->ptr);
		}
	} while (ring_ptr != &json->root->children);

	/* 遍历完所有结点 */

	it->ptr = it->data = NULL;
	return (NULL);
}

ACL_JSON *acl_json_alloc()
{
	return (acl_json_alloc1(NULL));
}

ACL_JSON *acl_json_alloc1(ACL_SLICE_POOL *slice)
{
	ACL_JSON *json;

	if (slice) {
		json = (ACL_JSON*) acl_slice_pool_calloc(__FILE__, __LINE__,
					slice, 1, sizeof(ACL_JSON));
		json->slice = slice;
	} else {
		json = (ACL_JSON*) acl_mycalloc(1, sizeof(ACL_JSON));
		json->slice = NULL;
	}

	json->root = acl_json_node_alloc(json);
	/* 将根结点作为当前结点 */
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
	return (json);
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
	ACL_JSON *json;
	ACL_JSON_NODE *root = node->json->root, *first;

	json = (ACL_JSON*) acl_mycalloc(1, sizeof(ACL_JSON));
	json->slice = NULL;

	/* 如果传入的结点为 root 结点，则直接赋值创建 root 即可 */
	if (node == root) {
		json->root = acl_json_node_duplicate(json, node);
	} else {
		json->root = acl_json_node_alloc(json);
		first = acl_json_node_duplicate(json, node);
		acl_json_node_add_child(json->root, first);
	}

	/* 将根结点作为当前结点 */
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

void acl_json_cache(ACL_JSON *json, int max_cache)
{
	if (json->node_cache != NULL) {
		acl_array_free(json->node_cache,
			(void (*)(void*)) acl_json_node_free);
		json->node_cache = NULL;
		json->max_cache = 0;
	}
	if (max_cache > 0) {
		json->node_cache = acl_array_create(max_cache);
		json->max_cache = max_cache;
	}
}

void acl_json_cache_free(ACL_JSON *json)
{
	if (json->node_cache != NULL) {
		acl_array_free(json->node_cache,
			(void (*)(void*)) acl_json_node_free);
		json->node_cache = NULL;
		json->max_cache = 0;
	} 
}

int acl_json_free(ACL_JSON *json)
{
	ACL_RING *next;
	ACL_JSON_NODE *node;
	int   n = 1;

	while ((next = acl_ring_pop_head(&json->root->children)) != NULL) {
		node = acl_ring_to_appl(next, ACL_JSON_NODE, node);
		n += acl_json_node_delete(node);
	}

	acl_json_node_free(json->root);
	json->node_cnt--;
	acl_assert(json->node_cnt == 0);
	if (json->node_cache != NULL)
		acl_array_free(json->node_cache,
			(void (*)(void*)) acl_json_node_free);
	if (json->slice)
		acl_slice_pool_free(__FILE__, __LINE__, json);
	else
		acl_myfree(json);
	return (n);
}

void acl_json_reset(ACL_JSON *json)
{
	const char *myname = "acl_json_reset";
	ACL_RING *next;
	ACL_JSON_NODE *node;

	while ((next = acl_ring_pop_head(&json->root->children)) != NULL) {
		node = acl_ring_to_appl(next, ACL_JSON_NODE, node);
		(void) acl_json_node_delete(node);
	}

	/* 因为根结点是一个虚结点，所以不需要释放，其会在调用
	 * acl_json_free 时被释放
	 */ 
	acl_ring_detach(&json->root->node);
	acl_json_node_reset(json->root);
	if (json->node_cnt != 1)
		acl_msg_fatal("%s(%d): node_cnt(%d) invalid",
			myname, __LINE__, json->node_cnt);

	json->root->left_ch = '{';
	json->root->right_ch = '}';
	json->root->type = ACL_JSON_T_OBJ;
	json->curr_node = json->root;
	json->status = ACL_JSON_S_ROOT;
	json->finish = 0;
	json->depth = 0;
}
