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
	attr->name       = node->xml->addr;
	attr->name_size  = 0;
	attr->value      = node->xml->addr;
	attr->value_size = 0;
	attr->quote      = 0;
	attr->backslash  = 0;

	acl_array_append(node->attr_list, attr);

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
	node->ltag      = xml->addr;
	node->rtag      = xml->addr;
	node->ltag_size = 0;
	node->rtag_size = 0;

	node->text      = xml->addr;
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

static ACL_XML2_NODE *xml_iter_head(ACL_ITER *it, ACL_XML2 *xml)
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

static ACL_XML2_NODE *xml_iter_next(ACL_ITER *it, ACL_XML2 *xml)
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

static ACL_XML2_NODE *xml_iter_tail(ACL_ITER *it, ACL_XML2 *xml)
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

static ACL_XML2_NODE *xml_iter_prev(ACL_ITER *it, ACL_XML2 *xml)
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

ACL_XML2 *acl_xml2_alloc(char *buf, size_t size)
{
	return acl_xml2_dbuf_alloc(buf, size, NULL);
}

ACL_XML2 *acl_xml2_mmap_file(const char *filepath, size_t size, size_t block,
	int keep_open, ACL_DBUF_POOL *dbuf)
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

	xml = acl_xml2_mmap_fd(fd, size, block, dbuf);
	if (xml == NULL) {
		acl_file_close(fd);
		return NULL;
	}

	if (!keep_open) {
		acl_file_close(fd);
		fd = ACL_FILE_INVALID;
	}

	xml->keep_open = keep_open;
	xml->mm_file   = acl_dbuf_pool_strdup(xml->dbuf, filepath);

	return xml;
}

ACL_XML2 *acl_xml2_mmap_fd(ACL_FILE_HANDLE fd, size_t size,
	size_t block, ACL_DBUF_POOL *dbuf)
{
#ifdef	ACL_UNIX
	size_t off = block - 1;
	ACL_XML2 *xml;
	char *addr;

	acl_assert(size > 0);
	acl_assert(block > 0);

	if (block > size)
		block = size;
	if (acl_lseek(fd, off, SEEK_SET) != (acl_off_t) off) {
		acl_msg_error("%s(%d), %s: lseek error: %s, block: %lu",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror(),
			(unsigned long) off);
		return NULL;
	}

	if (acl_file_write(fd, "\0", 1, 0, NULL, NULL) == ACL_VSTREAM_EOF)
	{
		acl_msg_error("%s(%d), %s: write error: %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		return NULL;
	}

	addr = (char*) mmap(NULL, size, PROT_READ | PROT_WRITE,
			MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		acl_msg_error("%s(%d), %s: mmap error: %s, size: %lu",
			__FILE__, __LINE__, __FUNCTION__,
			acl_last_serror(), (unsigned long) size);
		return NULL;
	}

	xml            = acl_xml2_dbuf_alloc(addr, size, dbuf);
	xml->mm_file   = NULL;
	xml->fd        = fd;
	xml->mm_addr   = addr;
	xml->block     = block;
	xml->off       = off;
	xml->keep_open = 1;
	xml->len       = xml->off + 1;

	return xml;
#else
	(void) fd;
	(void) size;
	(void) block;
	(void) dbuf;

	acl_msg_error("%s(%d), %s: not implement yet!",
		__FILE__, __LINE__, __FUNCTION__);
	return NULL;
#endif
}

size_t acl_xml2_mmap_extend(ACL_XML2 *xml)
{
	const char *myname = "acl_xml2_mmap_extend";
	size_t n;

	if (xml->ptr >= xml->addr + xml->size)
		return 0;

	if (xml->block == 0)
		return 0;

	if (xml->fd == ACL_FILE_INVALID) {
		if (xml->mm_file == NULL || *xml->mm_file == 0)
			return 0;

		xml->fd = acl_file_open(xml->mm_file, O_CREAT | O_RDWR, 0600);
		if (xml->fd == ACL_FILE_INVALID) {
			acl_msg_error("%s(%d), %s: open %s error: %s",
				__FILE__, __LINE__, myname,
				xml->mm_file, acl_last_serror());
			return 0;
		}
	}

	n = xml->size - xml->len;
	if (n > xml->block)
		n = xml->block;

	return acl_xml2_mmap_extend_size(xml, n);
}

size_t acl_xml2_mmap_extend_size(ACL_XML2 *xml, size_t size)
{
	const char *myname = "acl_xml2_mmap_extend_size";
	size_t n;

	if (size == 0)
		size = xml->block;

	if (xml->ptr >= xml->addr + xml->size)
		return 0;

	n = xml->size - xml->len;
	if (n > size)
		n = size;

	xml->len += n;
	xml->off += n;

	if (acl_lseek(xml->fd, xml->off, SEEK_SET) != (acl_off_t) xml->off)
	{
		acl_msg_error("%s(%d), %s: lseek error: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
		acl_file_close(xml->fd);
		xml->fd = ACL_FILE_INVALID;
		return 0;
	}

	if (acl_file_write(xml->fd, "\0", 1, 0, NULL, NULL) == ACL_VSTREAM_EOF)
	{
		acl_msg_error("%s(%d), %s: write error: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
		acl_file_close(xml->fd);
		xml->fd = ACL_FILE_INVALID;
		return 0;
	}

	if (!xml->keep_open) {
		acl_file_close(xml->fd);
		xml->fd = ACL_FILE_INVALID;
	}

	return n;
}

ACL_XML2 *acl_xml2_dbuf_alloc(char *buf, size_t size, ACL_DBUF_POOL *dbuf)
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

	xml->dbuf      = dbuf;
	xml->dbuf_keep = sizeof(ACL_XML2);
	xml->addr      = buf;
	xml->size      = size;
	xml->len       = size;
	xml->ptr       = xml->addr;
	*xml->ptr++    = 0;
	xml->flag     |= ACL_XML2_FLAG_MULTI_ROOT;

	xml->mm_file   = NULL;
	xml->fd        = ACL_FILE_INVALID;
	xml->off       = size - 1;
	xml->block     = size;

	xml->iter_head = xml_iter_head;
	xml->iter_next = xml_iter_next;
	xml->iter_tail = xml_iter_tail;
	xml->iter_prev = xml_iter_prev;

	xml->id_table  = acl_htable_create(100, 0);
	xml->root      = acl_xml2_node_alloc(xml);
	xml->node_cnt  = 1;

	return xml;
}

int acl_xml2_free(ACL_XML2 *xml)
{
	const char *myname = "acl_xml2_free";
	int  node_cnt = xml->node_cnt;

	acl_htable_free(xml->id_table, NULL);

	if (xml->fd != ACL_FILE_INVALID) {
		acl_file_close(xml->fd);
		xml->fd = ACL_FILE_INVALID;
	}

#ifdef	ACL_UNIX
	if (xml->mm_addr != NULL && munmap(xml->mm_addr, xml->size) < 0)
		acl_msg_error("%s(%d), %s: munmap error: %s",
			__FILE__, __LINE__, myname, acl_last_serror());
#endif

	if (xml->mm_file != NULL) {
		acl_dbuf_pool_free(xml->dbuf, xml->mm_file);
		xml->mm_file = NULL;
	}

	if (xml->dbuf_inner != NULL)
		acl_dbuf_pool_destroy(xml->dbuf_inner);

	return node_cnt - 1;
}

void acl_xml2_reset(ACL_XML2 *xml)
{
	acl_htable_reset(xml->id_table, NULL);

	if (xml->dbuf_inner != NULL)
		acl_dbuf_pool_reset(xml->dbuf_inner, xml->dbuf_keep);

	if (xml->fd != ACL_FILE_INVALID || xml->mm_file != NULL)
		xml->len = xml->off + 1;
	else
		xml->len = xml->size;

	xml->ptr       = xml->addr;
	xml->root      = acl_xml2_node_alloc(xml);
	xml->depth     = 0;
	xml->node_cnt  = 1;
	xml->root_cnt  = 0;
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
