/* C library */

#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

/* Local stuff */

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_slice.h"
#include "stdlib/acl_binhash.h"

#endif

/* binhash_iter_head */

static void *binhash_iter_head(ACL_ITER *iter, struct ACL_BINHASH *table)
{
	ACL_BINHASH_INFO *ptr = NULL;

	iter->dlen = -1;
	iter->i = 0;
	iter->size = table->size;
	iter->ptr = NULL;

	for (; iter->i < iter->size; iter->i++) {
		if (table->data[iter->i] != NULL) {
			iter->ptr = ptr = table->data[iter->i];
			break;
		}
	}

	if (ptr) {
		iter->data = ptr->value;
		iter->key = (const char*) ptr->key.c_key;
		iter->klen = ptr->key_len;
	} else {
		iter->data = NULL;
		iter->key = NULL;
		iter->klen = 0;
	}
	return (iter->ptr);
}

/* binhash_iter_next */

static void *binhash_iter_next(ACL_ITER *iter, struct ACL_BINHASH *table)
{
	ACL_BINHASH_INFO *ptr;

	ptr = (ACL_BINHASH_INFO*) iter->ptr;
	if (ptr) {
		iter->ptr = ptr = ptr->next;
		if (ptr) {
			iter->data = ptr->value;
			iter->key = (const char*) ptr->key.c_key;
			iter->klen = ptr->key_len;
			return (iter->ptr);
		}
	}

	for (iter->i++; iter->i < iter->size; iter->i++) {
		if (table->data[iter->i] != NULL) {
			iter->ptr = ptr = table->data[iter->i];
			break;
		}
	}

	if (ptr) {
		iter->data = ptr->value;
		iter->key = (const char*) ptr->key.c_key;
		iter->klen = ptr->key_len;
	} else {
		iter->data = NULL;
		iter->key = NULL;
		iter->klen = 0;
	}
	return (iter->ptr);
}

/* binhash_iter_tail */

static void *binhash_iter_tail(ACL_ITER *iter, struct ACL_BINHASH *table)
{
	ACL_BINHASH_INFO *ptr = NULL;

	iter->dlen = -1;
	iter->i = table->size - 1;
	iter->size = table->size;
	iter->ptr = NULL;

	for (; iter->i >= 0; iter->i--) {
		if (table->data[iter->i] != NULL) {
			iter->ptr = ptr = table->data[iter->i];
			break;
		}
	}

	if (ptr) {
		iter->data = ptr->value;
		iter->key = (const char*) ptr->key.c_key;
		iter->klen = ptr->key_len;
	} else {
		iter->data = NULL;
		iter->key = NULL;
		iter->klen = 0;
	}
	return (iter->ptr);
}

/* binhash_iter_prev */

static void *binhash_iter_prev(ACL_ITER *iter, struct ACL_BINHASH *table)
{
	ACL_BINHASH_INFO *ptr;

	ptr = (ACL_BINHASH_INFO*) iter->ptr;
	if (ptr) {
		iter->ptr = ptr = ptr->next;
		if (ptr != NULL) {
			iter->data = ptr->value;
			iter->key = (const char*) ptr->key.c_key;
			iter->klen = ptr->key_len;
			return (iter->ptr);
		}
	}

	for (iter->i--; iter->i >= 0; iter->i--) {
		if (table->data[iter->i] != NULL) {
			iter->ptr = ptr = table->data[iter->i];
			break;
		}
	}

	iter->data = ptr ? ptr->value : NULL;
	return (iter->ptr);
}

/* binhash_iter_info */

static ACL_BINHASH_INFO *binhash_iter_info(ACL_ITER *iter, struct ACL_BINHASH *table acl_unused)
{
	return (iter->ptr ? (ACL_BINHASH_INFO*) iter->ptr : NULL);
}

/* binhash_hash - hash a string */

static unsigned binhash_hash(const void *key_in, size_t len)
{
	const char *key = (const char *) key_in;
	unsigned long h = 0;
	unsigned long g;

	/*
	 * From the "Dragon" book by Aho, Sethi and Ullman.
	 */

	while (len-- > 0) {
		h = (h << 4) + *key++;
		if ((g = (h & 0xf0000000)) != 0) {
			h ^= (g >> 24);
			h ^= g;
		}
	}
	return (unsigned) h;
}

/* binhash_link - insert element into table */

#define binhash_link(_table, _element, _n) { \
	ACL_BINHASH_INFO **_h = _table->data + _n; \
	_element->prev = 0; \
	if ((_element->next = *_h) != 0) \
		(*_h)->prev = _element; \
	*_h = _element; \
	_table->used++; \
}

#define	KEY_EQ(x,y,l) (((const char*)x)[0] == ((const char*)y)[0] && memcmp(x,y,l) == 0)

/* binhash_size - allocate and initialize hash table */

static void binhash_size(ACL_BINHASH *table, unsigned size)
{
	ACL_BINHASH_INFO **h;

	size |= 1;

	table->data = h = (ACL_BINHASH_INFO **)
		acl_mymalloc(size * sizeof(ACL_BINHASH_INFO *));
	table->size = size;
	table->used = 0;

	while (size-- > 0)
		*h++ = 0;
}

/* acl_binhash_create - create initial hash table */

ACL_BINHASH *acl_binhash_create(int size, unsigned int flag)
{
	ACL_BINHASH *table;
	unsigned int slice_flag = 0;

	table = (ACL_BINHASH *) acl_mycalloc(1, sizeof(ACL_BINHASH));
	binhash_size(table, size < 13 ? 13 : size);
	table->flag = flag;
	table->hash_fn = binhash_hash;

	table->iter_head = binhash_iter_head;
	table->iter_next = binhash_iter_next;
	table->iter_tail = binhash_iter_tail;
	table->iter_prev = binhash_iter_prev;
	table->iter_info = binhash_iter_info;

	if ((flag & ACL_BINHASH_FLAG_SLICE1))
		slice_flag |= ACL_SLICE_FLAG_GC1;
	if ((flag & ACL_BINHASH_FLAG_SLICE2))
		slice_flag |= ACL_SLICE_FLAG_GC2;
	if ((flag & ACL_BINHASH_FLAG_SLICE3))
		slice_flag |= ACL_SLICE_FLAG_GC3;

	if (slice_flag) {
		if ((slice_flag & ACL_BINHASH_FLAG_SLICE_RTGC_OFF))
			slice_flag |= ACL_SLICE_FLAG_RTGC_OFF;
		table->slice = acl_slice_create("acl_binhash", 0,
			sizeof(ACL_BINHASH_INFO), slice_flag);
	}
	return (table);
}

/* acl_binhash_grow - extend existing table */

static void acl_binhash_grow(ACL_BINHASH *table)
{
	ACL_BINHASH_INFO *ht;
	ACL_BINHASH_INFO *next;
	unsigned old_size = table->size;
	ACL_BINHASH_INFO **h = table->data;
	ACL_BINHASH_INFO **old_entries = h;
	unsigned n;

	binhash_size(table, 2 * old_size);

	while (old_size-- > 0) {
		for (ht = *h++; ht; ht = next) {
			next = ht->next;
			n = table->hash_fn(ht->key.c_key, ht->key_len) % table->size;
			binhash_link(table, ht, n);
		}
	}
	acl_myfree(old_entries);
}

/* acl_binhash_enter - enter (key, value) pair */

ACL_BINHASH_INFO *acl_binhash_enter(ACL_BINHASH *table, const void *key, int key_len, void *value)
{
	ACL_BINHASH_INFO *ht;
	const char *p1, *p2;
	unsigned n;

	if (table->used >= table->size)
		acl_binhash_grow(table);

	n = table->hash_fn(key, key_len) % table->size;
	for (ht = table->data[n]; ht; ht = ht->next) {
		p1 = (const char*) key;
		p2 = (const char*) ht->key.c_key;

		if (key_len == ht->key_len && KEY_EQ(p1, p2, key_len)) {
			table->status = ACL_BINHASH_STAT_DUPLEX_KEY;
			return (ht);
		}
	}

	if (table->slice)
		ht = (ACL_BINHASH_INFO *) acl_slice_alloc(table->slice);
	else
		ht = (ACL_BINHASH_INFO *) acl_mymalloc(sizeof(ACL_BINHASH_INFO));

	if ((table->flag & ACL_BINHASH_FLAG_KEY_REUSE))
		ht->key.c_key = key;
	else
		ht->key.key = acl_mymemdup(key, key_len);
	ht->key_len = key_len;
	ht->value = value;
	binhash_link(table, ht, n);
	table->status = ACL_BINHASH_STAT_OK;
	return (ht);
}

/* acl_binhash_find - lookup value */

void  *acl_binhash_find(ACL_BINHASH *table, const void *key, int key_len)
{
	ACL_BINHASH_INFO *ht;
	unsigned n;

	n = table->hash_fn(key, key_len) % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (key_len == ht->key_len && KEY_EQ(key, ht->key.c_key, key_len)) {
			table->status = ACL_BINHASH_STAT_OK;
			return (ht->value);
		}
	}
	table->status = ACL_BINHASH_STAT_NO_KEY;
	return (0);
}

/* acl_binhash_locate - lookup entry */

ACL_BINHASH_INFO *acl_binhash_locate(ACL_BINHASH *table, const void *key, int key_len)
{
	ACL_BINHASH_INFO *ht;
	unsigned n;

	n = table->hash_fn(key, key_len) % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (key_len == ht->key_len && KEY_EQ(key, ht->key.c_key, key_len)) {
			table->status = ACL_BINHASH_STAT_OK;
			return (ht);
		}
	}
	table->status = ACL_BINHASH_STAT_NO_KEY;
	return (0);
}

/* acl_binhash_delete - delete one entry */

int acl_binhash_delete(ACL_BINHASH *table, const void *key, int key_len, void (*free_fn) (void *))
{
	ACL_BINHASH_INFO *ht, **h;
	unsigned n;

	n = table->hash_fn(key, key_len) % table->size;
	h = table->data + n;

	for (ht = *h; ht; ht = ht->next) {
		if (key_len == ht->key_len && KEY_EQ(key, ht->key.c_key, key_len)) {
			if (ht->next)
				ht->next->prev = ht->prev;
			if (ht->prev)
				ht->prev->next = ht->next;
			else
				*h = ht->next;
			if (free_fn)
				(*free_fn) (ht->value);
			if (!(table->flag & ACL_BINHASH_FLAG_KEY_REUSE))
				acl_myfree(ht->key.key);
			if (table->slice)
				acl_slice_free2(table->slice, ht);
			else
				acl_myfree(ht);
			table->used--;
			table->status = ACL_BINHASH_STAT_OK;
			return (0);
		}
	}
	table->status = ACL_BINHASH_STAT_NO_KEY;
	return (-1);
}

/* acl_binhash_free - destroy hash table */

void    acl_binhash_free(ACL_BINHASH *table, void (*free_fn) (void *))
{
	unsigned i = table->size;
	ACL_BINHASH_INFO *ht;
	ACL_BINHASH_INFO *next;
	ACL_BINHASH_INFO **h = table->data;

	while (i-- > 0) {
		for (ht = *h++; ht; ht = next) {
			next = ht->next;
			if (free_fn)
				(*free_fn) (ht->value);
			if (!(table->flag & ACL_BINHASH_FLAG_KEY_REUSE))
				acl_myfree(ht->key.key);
			if (!table->slice)
				acl_myfree(ht);
#if 0
			else
				acl_slice_free2(table->slice, ht);
#endif
		}
	}

	acl_myfree(table->data);
	if (table->slice)
		acl_slice_destroy(table->slice);
	acl_myfree(table);
}

/* acl_binhash_walk - iterate over hash table */

void    acl_binhash_walk(ACL_BINHASH *table,
	void (*action) (ACL_BINHASH_INFO *, void *), void *ptr)
{
	unsigned i = table->size;
	ACL_BINHASH_INFO **h = table->data;
	ACL_BINHASH_INFO *ht;

	while (i-- > 0)
		for (ht = *h++; ht; ht = ht->next)
			(*action) (ht, ptr);
}

/* acl_binhash_list - list all table members */

ACL_BINHASH_INFO **acl_binhash_list(ACL_BINHASH *table)
{
	ACL_BINHASH_INFO **list;
	ACL_BINHASH_INFO *member;
	int     count = 0;
	int     i;

	list = (ACL_BINHASH_INFO **) acl_mymalloc(sizeof(*list) * (table->used + 1));
	for (i = 0; i < table->size; i++)
		for (member = table->data[i]; member != 0; member = member->next)
			list[count++] = member;
	list[count] = 0;
	return (list);
}

int acl_binhash_errno(ACL_BINHASH *table)
{
	return (table->status);
}

int acl_binhash_size(const ACL_BINHASH *table)
{
	if (table)
		return (table->size);
	else
		return (0);
}

int acl_binhash_used(ACL_BINHASH *table)
{
	return (table->used);
}

ACL_BINHASH_INFO **acl_binhash_data(ACL_BINHASH *table)
{
	return ((ACL_BINHASH_INFO**) table->data);
}

const ACL_BINHASH_INFO *acl_binhash_iter_head(ACL_BINHASH *table, ACL_BINHASH_ITER *iter)
{
	iter->i = 0;
	iter->size = table->size;
	iter->h = table->data;
	iter->ptr = NULL;

	for (; iter->i < iter->size; iter->i++) {
		if (iter->h[iter->i] != 0) {
			iter->ptr = iter->h[iter->i];
			break;
		}
	}

	return (iter->ptr);
}

const ACL_BINHASH_INFO *acl_binhash_iter_next(ACL_BINHASH_ITER *iter)
{
	if (iter->ptr) {
		iter->ptr = iter->ptr->next;
		if (iter->ptr != NULL)
			return (iter->ptr);
	}

	for (iter->i++; iter->i < iter->size; iter->i++) {
		if (iter->h[iter->i] != 0) {
			iter->ptr = iter->h[iter->i];
			break;
		}
	}

	return (iter->ptr);
}

const ACL_BINHASH_INFO *acl_binhash_iter_tail(ACL_BINHASH *table, ACL_BINHASH_ITER *iter)
{
	iter->i = table->size - 1;
	iter->size = table->size;
	iter->h = table->data;
	iter->ptr = NULL;

	for (; iter->i >= 0; iter->i--) {
		if (iter->h[iter->i] != 0) {
			iter->ptr = iter->h[iter->i];
			break;
		}
	}

	return (iter->ptr);
}

const ACL_BINHASH_INFO *acl_binhash_iter_prev(ACL_BINHASH_ITER *iter)
{
	if (iter->ptr) {
		iter->ptr = iter->ptr->next;
		if (iter->ptr != NULL)
			return (iter->ptr);
	}

	for (iter->i--; iter->i >= 0; iter->i--) {
		if (iter->h[iter->i] != 0) {
			iter->ptr = iter->h[iter->i];
			break;
		}
	}

	return (iter->ptr);
}
