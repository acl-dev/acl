#include "StdAfx.h"
#include "stdlib/acl_define.h"


#ifndef ACL_PREPARE_COMPILE

#ifdef ACL_UNIX
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#endif

#include <string.h>
#include <stdio.h>
#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_define.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_dbuf_pool.h"
#include "xml/acl_xml2.h"

#endif

#define	LEN	ACL_VSTRING_LEN
#define	STR	acl_vstring_str

ACL_XML2_ATTR *acl_xml2_attr_alloc(ACL_XML2_NODE *node)
{
	ACL_XML2_ATTR *attr = (ACL_XML2_ATTR*)
		acl_dbuf_pool_calloc(node->xml->dbuf, sizeof(ACL_XML2_ATTR));

	attr->node       = node;
	attr->name       = node->xml->dummy;
	attr->name_size  = 0;
	attr->value      = node->xml->dummy;
	attr->value_size = 0;
	attr->quote      = 0;
	attr->backslash  = 0;

	acl_array_append(node->attr_list, attr);
	node->xml->attr_cnt++;

	return attr;
}

static ACL_XML2_NODE *node_iter_head(ACL_ITER *it, ACL_XML2_NODE *node)
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

	it->ptr = acl_ring_to_appl(ring_ptr, ACL_XML2_NODE, node);
	it->data = it->ptr;

	return it->ptr;
}

static ACL_XML2_NODE *node_iter_next(ACL_ITER *it, ACL_XML2_NODE *node)
{
	ACL_RING *ring_ptr;
	struct ACL_XML2_NODE *child = (struct ACL_XML2_NODE*) it->data;

	if ((ring_ptr = acl_ring_succ(&child->node)) == &node->children) {
		it->ptr = it->data = NULL;
		return NULL;
	}

	it->i++;
	it->ptr = acl_ring_to_appl(ring_ptr, ACL_XML2_NODE, node);
	it->data = it->ptr;

	return it->ptr;
}

static ACL_XML2_NODE *node_iter_tail(ACL_ITER *it, ACL_XML2_NODE *node)
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

	it->ptr = acl_ring_to_appl(ring_ptr, ACL_XML2_NODE, node);
	it->data = it->ptr;

	return it->ptr;
}

static ACL_XML2_NODE *node_iter_prev(ACL_ITER *it, ACL_XML2_NODE *node)
{
	ACL_RING *ring_ptr;
	struct ACL_XML2_NODE *child = (struct ACL_XML2_NODE*) it->data;

	if ((ring_ptr = acl_ring_pred(&child->node)) == &node->children) {
		it->ptr = it->data = NULL;
		return NULL;
	}

	it->i++;
	it->ptr = acl_ring_to_appl(ring_ptr, ACL_XML2_NODE, node);
	it->data = it->ptr;

	return it->ptr;
}

ACL_XML2_NODE *acl_xml2_node_alloc(ACL_XML2 *xml)
{
	ACL_XML2_NODE *node = (ACL_XML2_NODE*)
		acl_dbuf_pool_calloc(xml->dbuf, sizeof(ACL_XML2_NODE));

	acl_ring_init(&node->children);
	acl_ring_init(&node->node);

	node->xml       = xml;
	node->status    = ACL_XML2_S_NXT;
	node->ltag      = xml->dummy;
	node->rtag      = xml->dummy;
	node->ltag_size = 0;
	node->rtag_size = 0;

	node->text      = xml->dummy;
	node->text_size = 0;
	node->attr_list = acl_array_dbuf_create(100, xml->dbuf);

	node->iter_head = node_iter_head;
	node->iter_next = node_iter_next;
	node->iter_tail = node_iter_tail;
	node->iter_prev = node_iter_prev;

	return node;
}

int acl_xml2_node_delete(ACL_XML2_NODE *node)
{
	ACL_RING *next;
	ACL_XML2_NODE *node_next;
	int   n = 1;

	while ((next = acl_ring_pop_head(&node->children)) != NULL) {
		node_next = acl_ring_to_appl(next, ACL_XML2_NODE, node);
		n += acl_xml2_node_delete(node_next);
	}

	if (node->id != NULL)
		acl_htable_delete(node->xml->id_table, node->id, NULL);

	if (node->attr_list != NULL) {
		int k = acl_array_size(node->attr_list);
		if (node->xml->attr_cnt >= k)
			node->xml->attr_cnt -= k;
	}

	acl_ring_detach(&node->node);
	node->xml->node_cnt--;

	return n;
}

void acl_xml2_node_append(ACL_XML2_NODE *node1, ACL_XML2_NODE *node2)
{
	acl_ring_append(&node1->node, &node2->node);
	node2->parent = node1->parent;
	node1->xml->node_cnt++;
}

void acl_xml2_node_add_child(ACL_XML2_NODE *parent, ACL_XML2_NODE *child)
{
	acl_ring_prepend(&parent->children, &child->node);
	child->parent = parent;
	parent->xml->node_cnt++;
}

ACL_XML2_NODE *acl_xml2_node_parent(ACL_XML2_NODE *node)
{
	return node->parent;
}

ACL_XML2_NODE *acl_xml2_node_next(ACL_XML2_NODE *node)
{
	ACL_RING *ring_ptr = acl_ring_succ(&node->node);
	ACL_XML2_NODE *parent;

	if (ring_ptr == &node->node)
		return NULL;

	parent = node->parent;
	acl_assert(parent != NULL);
	if (ring_ptr == &parent->children)
		return NULL;

	return acl_ring_to_appl(ring_ptr, ACL_XML2_NODE, node);
}

ACL_XML2_NODE *acl_xml2_node_prev(ACL_XML2_NODE *node)
{
	ACL_RING *ring_ptr = acl_ring_pred(&node->node);
	ACL_XML2_NODE *parent;

	if (ring_ptr == &node->node)
		return NULL;
	parent = node->parent;
	acl_assert(parent != NULL);
	if (ring_ptr == &parent->children)
		return NULL;

	return acl_ring_to_appl(ring_ptr, ACL_XML2_NODE, node);
}

static ACL_XML2_NODE *xml_iter_head(ACL_ITER *it, const ACL_XML2 *xml)
{
	ACL_RING *ring_ptr;

	it->dlen = -1;
	it->key = NULL;
	it->klen = -1;

	it->i = 0;
	it->size = xml->node_cnt;

	ring_ptr = acl_ring_succ(&xml->root->children);
	if (ring_ptr== &xml->root->children) {
		it->ptr = it->data = NULL;
		return NULL;
	}

	it->ptr = acl_ring_to_appl(ring_ptr, ACL_XML2_NODE, node);
	it->data = it->ptr;

	return it->ptr;
}

static ACL_XML2_NODE *xml_iter_next(ACL_ITER *it, const ACL_XML2 *xml)
{
	ACL_RING *ring_ptr;
	struct ACL_XML2_NODE *node, *parent;

	node = (struct ACL_XML2_NODE*) it->data;

	/* 先遍历当前节点的子节点 */

	ring_ptr = acl_ring_succ(&node->children);
	if (ring_ptr != &node->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, ACL_XML2_NODE, node);
		it->data = it->ptr;
		return it->ptr;
	}

	/* 当前节点的子节点遍历完毕，再遍历当前节点的兄弟节点 */

	parent = acl_xml2_node_parent(node);
	ring_ptr = acl_ring_succ(&node->node);
	if (ring_ptr != &parent->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, ACL_XML2_NODE, node);
		it->data = it->ptr;
		return it->ptr;
	}

	/* 当前节点的兄弟节点遍历完毕，最后遍历当前节点的父节点的兄弟节点 */

	do {
		if (parent == xml->root)
			break;

		ring_ptr = acl_ring_succ(&parent->node);
		parent = acl_xml2_node_parent(parent);
		if (parent == NULL)
			acl_msg_fatal("%s(%d): parent null",
				__FILE__, __LINE__);

		if (ring_ptr != &parent->children) {
			it->i++;
			it->ptr = acl_ring_to_appl(ring_ptr,
					ACL_XML2_NODE, node);
			it->data = it->ptr;
			return it->ptr;
		}
	} while (ring_ptr != &xml->root->children);

	/* 遍历完所有节点 */

	it->ptr = it->data = NULL;
	return NULL;
}

static ACL_XML2_NODE *xml_iter_tail(ACL_ITER *it, const ACL_XML2 *xml)
{
	ACL_RING *ring_ptr;

	it->dlen = -1;
	it->key = NULL;
	it->klen = -1;

	it->i = 0;
	it->size = xml->node_cnt;

	ring_ptr = acl_ring_pred(&xml->root->children);
	if (ring_ptr== &xml->root->children) {
		it->ptr = it->data = NULL;
		return NULL;
	}
	it->ptr = acl_ring_to_appl(ring_ptr, ACL_XML2_NODE, node);
	it->data = it->ptr;
	return it->ptr;
}

static ACL_XML2_NODE *xml_iter_prev(ACL_ITER *it, const ACL_XML2 *xml)
{
	ACL_RING *ring_ptr;
	struct ACL_XML2_NODE *node, *parent;

	node = (struct ACL_XML2_NODE*) it->data;

	/* 先遍历当前节点的子节点 */

	ring_ptr = acl_ring_pred(&node->children);
	if (ring_ptr != &node->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, ACL_XML2_NODE, node);
		it->data = it->ptr;
		return it->ptr;
	}

	/* 当前节点的子节点遍历完毕，再遍历当前节点的兄弟节点 */

	parent = acl_xml2_node_parent(node);
	ring_ptr = acl_ring_pred(&node->node);
	if (ring_ptr != &parent->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, ACL_XML2_NODE, node);
		it->data = it->ptr;
		return it->ptr;
	}

	/* 当前节点的兄弟节点遍历完毕，最后遍历当前节点的父节点的兄弟节点 */

	do {
		if (parent == xml->root)
			break;
		ring_ptr = acl_ring_pred(&parent->node);
		parent = acl_xml2_node_parent(parent);
		if (parent == NULL)
			acl_msg_fatal("%s(%d): parent null",
				__FILE__, __LINE__);

		if (ring_ptr != &parent->children) {
			it->i++;
			it->ptr = acl_ring_to_appl(ring_ptr,
					ACL_XML2_NODE, node);
			it->data = it->ptr;
			return it->ptr;
		}
	} while (ring_ptr != &xml->root->children);

	/* 遍历完所有节点 */

	it->ptr = it->data = NULL;
	return NULL;
}

void acl_xml2_multi_root(ACL_XML2 *xml, int on)
{
	if (on)
		xml->flag |= ACL_XML2_FLAG_MULTI_ROOT;
	else
		xml->flag &= ~ACL_XML2_FLAG_MULTI_ROOT;
}

void acl_xml2_slash(ACL_XML2 *xml, int ignore)
{
	if (ignore)
		xml->flag |= ACL_XML2_FLAG_IGNORE_SLASH;
	else
		xml->flag &=~ACL_XML2_FLAG_IGNORE_SLASH;
}

void acl_xml2_decode_enable(ACL_XML2 *xml, int on)
{
	if (on)
		xml->flag |= ACL_XML2_FLAG_XML_DECODE;
	else
		xml->flag &= ~ACL_XML2_FLAG_XML_DECODE;
}

void acl_xml2_encode_enable(ACL_XML2 *xml, int on)
{
	if (on)
		xml->flag |= ACL_XML2_FLAG_XML_ENCODE;
	else
		xml->flag &=~ACL_XML2_FLAG_XML_ENCODE;
}

ACL_XML2 *acl_xml2_mmap_file(const char *filepath, size_t max_len,
	size_t init_len, ACL_DBUF_POOL *dbuf)
{
	const char *myname = "acl_xml2_mmap_alloc";
	ACL_FILE_HANDLE fd;
	ACL_XML2 *xml;

	acl_assert(filepath && *filepath);

	fd = acl_file_open(filepath, O_CREAT | O_RDWR, 0600);
	if (fd == ACL_FILE_INVALID) {
		acl_msg_error("%s(%d), %s: open %s error: %s", __FILE__,
			__LINE__, myname, filepath, acl_last_serror());
		return NULL;
	}

	xml = acl_xml2_mmap_fd(fd, max_len, init_len, dbuf);
	if (xml == NULL) {
		acl_file_close(fd);
		return NULL;
	}

	/* save the fd will be closed in acl_vstring_free */
	xml->fd = fd;

	return xml;
}

ACL_XML2 *acl_xml2_mmap_fd(ACL_FILE_HANDLE fd, size_t max_len,
	size_t init_len, ACL_DBUF_POOL *dbuf)
{
	ACL_XML2 *xml;
	ACL_VSTRING *vbuf = acl_vstring_mmap_alloc(fd, (ssize_t) max_len,
		(ssize_t) init_len);

	if (vbuf == NULL)
		return NULL;

	xml = acl_xml2_dbuf_alloc(vbuf, dbuf);
	xml->vbuf_inner = vbuf;
	return xml;
}

ACL_XML2 *acl_xml2_alloc(ACL_VSTRING *buf)
{
#ifdef ACL_WINDOWS
	if (buf->vbuf.fd == ACL_FILE_INVALID)
#else
	if (buf->vbuf.fd < 0)
#endif
		buf->vbuf.flags |= ACL_VBUF_FLAG_FIXED;

	return acl_xml2_dbuf_alloc(buf, NULL);
}

ACL_XML2 *acl_xml2_dbuf_alloc(ACL_VSTRING *vbuf, ACL_DBUF_POOL *dbuf)
{
	ACL_XML2 *xml;

	if (dbuf == NULL) {
		dbuf = acl_dbuf_pool_create(8192);
		xml = (ACL_XML2*) acl_dbuf_pool_calloc(dbuf, sizeof(ACL_XML2));
		xml->dbuf_inner = dbuf;
	} else {
		xml = (ACL_XML2*) acl_dbuf_pool_calloc(dbuf, sizeof(ACL_XML2));
		xml->dbuf_inner = NULL;
	}

	xml->fd         = ACL_FILE_INVALID;
	xml->dbuf       = dbuf;
	xml->vbuf       = vbuf;
	xml->vbuf_inner = NULL;
	xml->dummy[0]   = '\0';
	xml->dbuf_keep  = sizeof(ACL_XML2);
	xml->flag       = ACL_XML2_FLAG_MULTI_ROOT |
			  ACL_XML2_FLAG_XML_ENCODE |
			  ACL_XML2_FLAG_XML_DECODE;

	xml->iter_head  = xml_iter_head;
	xml->iter_next  = xml_iter_next;
	xml->iter_tail  = xml_iter_tail;
	xml->iter_prev  = xml_iter_prev;

	xml->id_table   = acl_htable_create(100, 0);
	xml->root       = acl_xml2_node_alloc(xml);
	xml->node_cnt   = 1;
	xml->attr_cnt   = 0;

	return xml;
}

size_t acl_xml2_space(ACL_XML2 *xml)
{
	return LEN(xml->vbuf);
}

void acl_xml2_space_clear(ACL_XML2 *xml)
{
	ACL_VSTRING_RESET(xml->vbuf);
}

int acl_xml2_free(ACL_XML2 *xml)
{
	int  node_cnt = xml->node_cnt;

	acl_htable_free(xml->id_table, NULL);

#ifdef ACL_UNIX
	if (xml->fd >= 0)
#else
	if (xml->fd != ACL_FILE_INVALID)
#endif
		acl_file_close(xml->fd);

	if (xml->vbuf_inner != NULL)
		acl_vstring_free(xml->vbuf_inner);

	if (xml->dbuf_inner != NULL)
		acl_dbuf_pool_destroy(xml->dbuf_inner);

	return node_cnt - 1;
}

void acl_xml2_reset(ACL_XML2 *xml)
{
	acl_htable_reset(xml->id_table, NULL);

	if (xml->dbuf_inner != NULL)
		acl_dbuf_pool_reset(xml->dbuf_inner, xml->dbuf_keep);

	ACL_VSTRING_RESET(xml->vbuf);
	xml->root      = acl_xml2_node_alloc(xml);
	xml->depth     = 0;
	xml->node_cnt  = 1;
	xml->root_cnt  = 0;
	xml->attr_cnt  = 0;
	xml->curr_node = NULL;
}

void acl_xml2_foreach_init(ACL_XML2 *xml, ACL_XML2_NODE *node)
{
	xml->root = node;
	xml->iter_head = xml_iter_head;
	xml->iter_next = xml_iter_next;
	xml->iter_tail = xml_iter_tail;
	xml->iter_prev = xml_iter_prev;
}

int acl_xml2_is_closure(ACL_XML2 *xml)
{
	ACL_RING *ring_ptr;
	ACL_XML2_NODE *node;

	/* 获得 xml->root 节点的最后一个一级子节点 */
	ring_ptr = acl_ring_succ(&xml->root->children);

	if (ring_ptr == &xml->root->children) {
		/* 说明没有真实子节点 */
		return 0;
	}

	node = acl_ring_to_appl(ring_ptr, ACL_XML2_NODE, node);

	if ((node->flag & ACL_XML2_F_SELF_CL)) {
		/* 说明该节点是自闭合节点 */
		return 1;
	}

	if (node->status == ACL_XML2_S_RGT) {
		return 1;
	}

	/* 说明最后一个一级子节点还未处理完毕 */
	return 0;
}

int acl_xml2_is_complete(ACL_XML2 *xml, const char *tag)
{
	ACL_XML2_NODE *last_node = NULL;
	ACL_ITER iter;

	if (!(xml->flag & ACL_XML2_FLAG_MULTI_ROOT) && xml->root_cnt > 0)
		return 1;

	acl_foreach_reverse(iter, xml->root) {
		ACL_XML2_NODE *node = (ACL_XML2_NODE*) iter.data;
		if (!(node->flag & ACL_XML2_F_META)) {
			last_node = node;
			break;
		}
	}

	if (last_node == NULL)
		/* 说明没有真实子节点 */
		return 0;

	if ((last_node->flag & ACL_XML2_F_SELF_CL))
		/* 说明该节点是自闭合节点 */
		return 1;

	if (last_node->status != ACL_XML2_S_RGT)
		/* 说明最后一个一级子节点还未处理完毕 */
		return 0;

	if (strcasecmp(last_node->rtag, tag) == 0)
		return 1;

	/* 说明 xml 中的最后一个节点与所给标签不匹配 */
	return 0;
}
