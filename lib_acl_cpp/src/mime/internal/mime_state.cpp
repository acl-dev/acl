#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "header_opts.hpp"
#include "mime_state.hpp"

static MIME_NODE *node_iter_head(ACL_ITER *it, MIME_NODE *node)
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
	it->ptr = acl_ring_to_appl(ring_ptr, MIME_NODE, node);
	it->data = it->ptr;
	return ((MIME_NODE*) it->ptr);
}

static MIME_NODE *node_iter_next(ACL_ITER *it, MIME_NODE *node)
{
	ACL_RING *ring_ptr;
	MIME_NODE *child;

	child = (MIME_NODE*) it->data;
	if ((ring_ptr = acl_ring_succ(&child->node)) == &node->children) {
		it->ptr = it->data = NULL;
		return (NULL);
	}
	it->i++;
	it->ptr = acl_ring_to_appl(ring_ptr, MIME_NODE, node);
	it->data = it->ptr;
	return ((MIME_NODE*) it->ptr);
}

static MIME_NODE *node_iter_tail(ACL_ITER *it, MIME_NODE *node)
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

	it->ptr = acl_ring_to_appl(ring_ptr, MIME_NODE, node);
	it->data = it->ptr;
	return ((MIME_NODE*) it->ptr);
}

static MIME_NODE *node_iter_prev(ACL_ITER *it, MIME_NODE *node)
{
	ACL_RING *ring_ptr;
	MIME_NODE *child;

	child = (MIME_NODE*) it->data;
	if ((ring_ptr = acl_ring_pred(&child->node)) == &node->children) {
		it->ptr = it->data = NULL;
		return (NULL);
	}
	it->i++;
	it->ptr = acl_ring_to_appl(ring_ptr, MIME_NODE, node);
	it->data = it->ptr;
	return ((MIME_NODE*) it->ptr);
}

MIME_NODE *mime_node_new(MIME_STATE *state)
{
	MIME_NODE *node = (MIME_NODE*) acl_mycalloc(1, sizeof(MIME_NODE));

	node->buffer = acl_vstring_alloc(256);
	node->state = state;
	acl_ring_init(&node->children);
	acl_ring_init(&node->node);
	node->header_list = acl_fifo_new();

	state->node_cnt++;

	node->iter_head = node_iter_head;
	node->iter_next = node_iter_next;
	node->iter_tail = node_iter_tail;
	node->iter_prev = node_iter_prev;

	node->last_cr_pos = -1;
	node->last_lf_pos = -1;
	return (node);
}

static void mail_addr_list_free(ACL_FIFO *mail_addr_list)
{
	MAIL_ADDR *mail_addr;

	while(1) {
		mail_addr = (MAIL_ADDR*) acl_fifo_pop(mail_addr_list);
		if (mail_addr == NULL)
			break;
		if (mail_addr->addr)
			acl_myfree(mail_addr->addr);
		if (mail_addr->comment)
			acl_myfree(mail_addr->comment);
		acl_myfree(mail_addr);
	}
	acl_fifo_free(mail_addr_list, NULL);
}

static void mime_node_free(MIME_NODE *node)
{
	if (node->header_list) {
		HEADER_NV *header;
		while (1) {
			header = (HEADER_NV*) acl_fifo_pop(node->header_list);
			if (header == NULL)
				break;
			header_nv_free(header);
		}
		acl_fifo_free(node->header_list, NULL);
	}
	if (node->header_to_list)
		mail_addr_list_free(node->header_to_list);
	if (node->header_cc_list)
		mail_addr_list_free(node->header_cc_list);
	if (node->header_bcc_list)
		mail_addr_list_free(node->header_bcc_list);
	if (node->header_sender)
		acl_myfree(node->header_sender);
	if (node->header_from)
		acl_myfree(node->header_from);
	if (node->header_replyto)
		acl_myfree(node->header_replyto);
	if (node->header_returnpath)
		acl_myfree(node->header_returnpath);
	if (node->header_subject)
		acl_myfree(node->header_subject);

	if (node->header_filename)
		acl_myfree(node->header_filename);
	if (node->header_name)
		acl_myfree(node->header_name);
	if (node->charset)
		acl_myfree(node->charset);

	if (node->ctype_s)
		acl_myfree(node->ctype_s);
	if (node->stype_s)
		acl_myfree(node->stype_s);

	acl_vstring_free(node->buffer);
	if (node->boundary)
		acl_vstring_free(node->boundary);
	acl_myfree(node);
}

int mime_node_delete(MIME_NODE *node)
{
	ACL_RING *next;
	MIME_NODE *node_next;
	int   n = 1;

	while ((next = acl_ring_pop_head(&node->children)) != NULL) {
		node_next = acl_ring_to_appl(next, MIME_NODE, node);
		n += mime_node_delete(node_next);
	}

	node->state->node_cnt--;
	mime_node_free(node);
	return (n);
}

void mime_node_add_child(MIME_NODE *parent, MIME_NODE *child)
{
	acl_ring_prepend(&parent->children, &child->node);
	child->parent = parent;
}

/*-------------------------------------------------------------------------*/

static MIME_NODE *mime_iter_head(ACL_ITER *it, MIME_STATE *state)
{
#if 0
	ACL_RING *ring_ptr;

	it->dlen = -1;
	it->key = NULL;
	it->klen = -1;

	it->i = 0;
	it->size = state->node_cnt;

	ring_ptr = acl_ring_succ(&state->root->children);
	if (ring_ptr== &state->root->children) {
		it->ptr = it->data = NULL;
		return (NULL);
	}
	it->ptr = acl_ring_to_appl(ring_ptr, MIME_NODE, node);
	it->data = it->ptr;
	return ((MIME_NODE*) it->ptr);
#else
	it->dlen = -1;
	it->key = NULL;
	it->klen = -1;

	it->i = 0;
	it->size = state->node_cnt;

	it->data = it->ptr = state->root;
	return ((MIME_NODE*) it->data);
#endif
}

static MIME_NODE *mime_iter_next(ACL_ITER *it, MIME_STATE *state)
{
	ACL_RING *ring_ptr;
	MIME_NODE *node, *parent;

	node = (MIME_NODE*) it->data;

	/* 先遍历当前结点的子结点 */

	ring_ptr = acl_ring_succ(&node->children);
	if (ring_ptr != &node->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, MIME_NODE, node);
		it->data = it->ptr;
		return ((MIME_NODE*) it->ptr);
	}

	/* 如果当前结点是根结点则直接返回空 */

	if (node == state->root) {
		it->ptr = it->data = NULL;
		return (NULL);
	}

	/* 当前结点的子结点遍历完毕，再遍历当前结点的兄弟结点 */

	parent = node->parent;
	ring_ptr = acl_ring_succ(&node->node);
	if (ring_ptr != &parent->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, MIME_NODE, node);
		it->data = it->ptr;
		return ((MIME_NODE*) it->ptr);
	}

	/* 当前结点的兄弟结点遍历完毕，最后遍历当前结点的父结点的兄弟结点 */

	do {
		ring_ptr = acl_ring_succ(&parent->node);
		parent = parent->parent;
		if (parent == NULL)
			break;

		if (ring_ptr != &parent->children) {
			it->i++;
			it->ptr = acl_ring_to_appl(ring_ptr, MIME_NODE, node);
			it->data = it->ptr;
			return ((MIME_NODE*) it->ptr);
		}
	} while (ring_ptr != &state->root->children);

	/* 遍历完所有结点 */

	it->ptr = it->data = NULL;
	return (NULL);
}

static MIME_NODE *mime_iter_tail(ACL_ITER *it, MIME_STATE *state)
{
#if 0
	ACL_RING *ring_ptr;

	it->dlen = -1;
	it->key = NULL;
	it->klen = -1;

	it->i = 0;
	it->size = state->node_cnt;

	ring_ptr = acl_ring_pred(&state->root->children);
	if (ring_ptr== &state->root->children) {
		it->ptr = it->data = NULL;
		return (NULL);
	}
	it->ptr = acl_ring_to_appl(ring_ptr, MIME_NODE, node);
	it->data = it->ptr;
	return ((MIME_NODE*) it->ptr);
#else
	it->dlen = -1;
	it->key = NULL;
	it->klen = -1;

	it->i = 0;
	it->size = state->node_cnt;

	it->data = it->ptr = state->root;
	return ((MIME_NODE*) it->ptr);
#endif
}

static MIME_NODE *mime_iter_prev(ACL_ITER *it, MIME_STATE *state)
{
	ACL_RING *ring_ptr;
	MIME_NODE *node, *parent;

	node = (MIME_NODE*) it->data;

	/* 先遍历当前结点的子结点 */

	ring_ptr = acl_ring_pred(&node->children);
	if (ring_ptr != &node->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, MIME_NODE, node);
		it->data = it->ptr;
		return ((MIME_NODE*) it->ptr);
	}

	/* 如果当前结点是根结点则直接返回空 */

	if (node == state->root) {
		it->ptr = it->data = NULL;
		return (NULL);
	}

	/* 当前结点的子结点遍历完毕，再遍历当前结点的兄弟结点 */

	parent = node->parent;
	ring_ptr = acl_ring_pred(&node->node);
	if (ring_ptr != &parent->children) {
		it->i++;
		it->ptr = acl_ring_to_appl(ring_ptr, MIME_NODE, node);
		it->data = it->ptr;
		return ((MIME_NODE*) it->ptr);
	}

	/* 当前结点的兄弟结点遍历完毕，最后遍历当前结点的父结点的兄弟结点 */

	do {
		ring_ptr = acl_ring_pred(&parent->node);
		parent = parent->parent;
		if (parent == NULL)
			break;

		if (ring_ptr != &parent->children) {
			it->i++;
			it->ptr = acl_ring_to_appl(ring_ptr, MIME_NODE, node);
			it->data = it->ptr;
			return ((MIME_NODE*) it->ptr);
		}
	} while (ring_ptr != &state->root->children);

	/* 遍历完所有结点 */

	it->ptr = it->data = NULL;
	return (NULL);
}

void mime_state_foreach_init(MIME_STATE *state)
{
	state->iter_head = mime_iter_head;
	state->iter_next = mime_iter_next;
	state->iter_tail = mime_iter_tail;
	state->iter_prev = mime_iter_prev;
}

MIME_STATE *mime_state_alloc()
{
	MIME_STATE *state;

	state = (MIME_STATE*) acl_mycalloc(1, sizeof(MIME_STATE));
	state->root = mime_node_new(state);
	state->curr_node = state->root;
	state->curr_status = MIME_S_HEAD;
	state->token_buffer = acl_vstring_alloc(256);
	state->key_buffer = acl_vstring_alloc(128);

	state->iter_head = mime_iter_head;
	state->iter_next = mime_iter_next;
	state->iter_tail = mime_iter_tail;
	state->iter_prev = mime_iter_prev;

	return (state);
}

int mime_state_free(MIME_STATE *state)
{
	ACL_RING *next;
	MIME_NODE *node;
	int   n = 1;

	while ((next = acl_ring_pop_head(&state->root->children)) != NULL) {
		node = acl_ring_to_appl(next, MIME_NODE, node);
		n += mime_node_delete(node);
	}

	mime_node_free(state->root);
	state->node_cnt--;
	n = state->node_cnt;
	acl_vstring_free(state->token_buffer);
	acl_vstring_free(state->key_buffer);
	acl_myfree(state);

	return (n);
}

int mime_state_reset(MIME_STATE *state)
{
	ACL_RING *next;
	MIME_NODE *node;
	int   n;

	while ((next = acl_ring_pop_head(&state->root->children)) != NULL) {
		node = acl_ring_to_appl(next, MIME_NODE, node);
		(void) mime_node_delete(node);
	}

	mime_node_free(state->root);
	state->node_cnt--;
	n = state->node_cnt;

	state->root = mime_node_new(state);
	state->curr_node = state->root;
	state->curr_status = MIME_S_HEAD;
	ACL_VSTRING_RESET(state->token_buffer);

	state->depth = 0;
	state->curr_bound = NULL;
	state->curr_off = 0;
	return (n);
}

int mime_state_head_finish(MIME_STATE *state)
{
	if (state->curr_node != state->root)
		return (1);
	if (state->curr_status > MIME_S_HEAD)
		return (1);
	return (0);
}
/*-------------------------------------------------------------------------*/

typedef struct {
	int  type;
	const char *name;
} MIME_TYPE;

static MIME_TYPE mime_ctype_map[] = {
	{ MIME_CTYPE_OTHER, "other" },
	{ MIME_CTYPE_TEXT, "text" },
	{ MIME_CTYPE_MESSAGE, "message" },
	{ MIME_CTYPE_MULTIPART, "multipart" },
	{ MIME_CTYPE_IMAGE, "image" },
	{ MIME_CTYPE_APPLICATION, "application" },
};

static MIME_TYPE mime_stype_map[] = {
	{ MIME_STYPE_OTHER, "other" },
	{ MIME_STYPE_PLAIN, "plain" },
	{ MIME_STYPE_HTML, "html" },
	{ MIME_STYPE_RFC822, "rfc822" },
	{ MIME_STYPE_PARTIAL, "partial" },
	{ MIME_STYPE_EXTERN_BODY, "extern body" },
	{ MIME_STYPE_JPEG, "jpeg" },
	{ MIME_STYPE_GIF, "gif" },
	{ MIME_STYPE_BMP, "bmp" },
	{ MIME_STYPE_PNG, "png" },
	{ MIME_STYPE_OCTET_STREAM, "octet-stream" },
	{ MIME_STYPE_MIXED, "mixed" },
	{ MIME_STYPE_ALTERNATIVE, "alternative" },
	{ MIME_STYPE_RELATED, "related" },
};

#define OTHER_NAME	"other"

const char *mime_ctype_name(size_t ctype)
{
	if (ctype > MIME_CTYPE_MAX)
		return (OTHER_NAME);
	return (mime_ctype_map[ctype].name);
}

const char *mime_stype_name(size_t stype)
{
	if (stype > MIME_STYPE_MAX || stype < MIME_STYPE_MIN)
		return (OTHER_NAME);
	return (mime_stype_map[stype - MIME_STYPE_MIN].name);
}

const char *mime_head_value(MIME_NODE* node, const char* name)
{
	ACL_ITER iter;
	acl_foreach(iter, node->header_list)
	{
		HEADER_NV* hdr = (HEADER_NV*) iter.data;
		if (strcasecmp(hdr->name, name) == 0 && *hdr->value)
			return (hdr->value);
	}

	return NULL;
}
