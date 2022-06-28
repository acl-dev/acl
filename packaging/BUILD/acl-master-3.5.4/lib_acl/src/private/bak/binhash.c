/* C library */

#include "stdlib/acl_define.h"

#include <string.h>

#ifdef BCB_COMPILER
#pragma hdrstop
#endif

/* Local stuff */

#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_slice.h"
#include "binhash.h"

/**
 * Structure of one hash table.
 */
struct BINHASH {
	int     size;               /**< length of entries array */
	int     used;               /**< number of entries in table */
	BINHASH_INFO **data;        /**< entries array, auto-resized */
	unsigned int flag;
	ACL_SLICE *slice;
};

/* binhash_hash - hash a string */

static unsigned binhash_hash(const char *key, int len, unsigned size)
{
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
	return (h % size);
}

/* binhash_link - insert element into table */

#define binhash_link(table, elm) { \
	BINHASH_INFO **_h = table->data + binhash_hash(elm->key.c_key, elm->key_len, table->size);\
	elm->prev = 0; \
	if ((elm->next = *_h) != 0) \
		(*_h)->prev = elm; \
	*_h = elm; \
	table->used++; \
}

/* binhash_size - allocate and initialize hash table */

static void binhash_size(BINHASH *table, unsigned size)
{
	BINHASH_INFO **h;

	size |= 1;

	table->data = h = (BINHASH_INFO **) acl_mymalloc(size * sizeof(BINHASH_INFO *));
	table->size = size;
	table->used = 0;

	while (size-- > 0)
		*h++ = 0;
}

/* binhash_create - create initial hash table */

BINHASH *binhash_create(int size, unsigned int flag, int slice_type)
{
	BINHASH *table;

	table = (BINHASH *) acl_mycalloc(1, sizeof(BINHASH));
	binhash_size(table, size < 13 ? 13 : size);
	table->flag = flag;
	if (slice_type)
		table->slice = acl_slice_create("binhash", 0,
					sizeof(BINHASH_INFO), slice_type);
	return (table);
}

/* binhash_grow - extend existing table */

static void binhash_grow(BINHASH *table)
{
	BINHASH_INFO *ht;
	BINHASH_INFO *next;
	unsigned old_size = table->size;
	BINHASH_INFO **h = table->data;
	BINHASH_INFO **old_entries = h;

	binhash_size(table, 2 * old_size);

	while (old_size-- > 0) {
		for (ht = *h++; ht; ht = next) {
			next = ht->next;
			binhash_link(table, ht);
		}
	}
	acl_myfree(old_entries);
}

/* binhash_enter - enter (key, value) pair */

BINHASH_INFO *binhash_enter(BINHASH *table, const char *key, int key_len, char *value)
{
	BINHASH_INFO *ht;

	if (table->used >= table->size)
		binhash_grow(table);
	if (table->slice)
		ht = (BINHASH_INFO*) acl_slice_alloc(table->slice);
	else
		ht = (BINHASH_INFO *) acl_mymalloc(sizeof(BINHASH_INFO));
	if (table->flag & KEY_REUSE)
		ht->key.c_key = key;
	else
		ht->key.key = acl_mymemdup(key, key_len);
	ht->key_len = key_len;
	ht->value = value;
	binhash_link(table, ht);
	return (ht);
}

/* binhash_find - lookup value */

char   *binhash_find(BINHASH *table, const char *key, int key_len)
{
	BINHASH_INFO *ht;

#define	KEY_EQ(x,y,l) (x[0] == y[0] && memcmp(x,y,l) == 0)

	if (table != 0)
		for (ht = table->data[binhash_hash(key, key_len, table->size)]; ht; ht = ht->next)
			if (key_len == ht->key_len && KEY_EQ(key, ht->key.c_key, key_len))
				return (ht->value);
	return (0);
}

/* binhash_locate - lookup entry */

BINHASH_INFO *binhash_locate(BINHASH *table, const char *key, int key_len)
{
	BINHASH_INFO *ht;

#define	KEY_EQ(x,y,l) (x[0] == y[0] && memcmp(x,y,l) == 0)

	if (table != 0)
		for (ht = table->data[binhash_hash(key, key_len, table->size)]; ht; ht = ht->next)
			if (key_len == ht->key_len && KEY_EQ(key, ht->key.c_key, key_len))
				return (ht);
	return (0);
}

/* binhash_delete - delete one entry */

void    binhash_delete(BINHASH *table, const char *key, int key_len, void (*free_fn) (char *))
{
	if (table != 0) {
		BINHASH_INFO *ht;
		BINHASH_INFO **h = table->data + binhash_hash(key, key_len, table->size);

#define	KEY_EQ(x,y,l) (x[0] == y[0] && memcmp(x,y,l) == 0)

		for (ht = *h; ht; ht = ht->next) {
			if (key_len == ht->key_len && KEY_EQ(key, ht->key.c_key, key_len)) {
				if (ht->next)
					ht->next->prev = ht->prev;
				if (ht->prev)
					ht->prev->next = ht->next;
				else
					*h = ht->next;
				table->used--;
				if (free_fn)
					(*free_fn) (ht->value);
				if (!(table->flag & KEY_REUSE))
					acl_myfree(ht->key.key);
				if (table->slice)
					acl_slice_free(/*table->slice, */ht);
				else
					acl_myfree(ht);
				return;
			}
		}
		acl_msg_warn("binhash_delete: unknown_key: \"%s\"", key);
	}
}

/* binhash_free - destroy hash table */

void    binhash_free(BINHASH *table, void (*free_fn) (char *))
{
	if (table != 0) {
		unsigned i = table->size;
		BINHASH_INFO *ht;
		BINHASH_INFO *next;
		BINHASH_INFO **h = table->data;

		while (i-- > 0) {
			for (ht = *h++; ht; ht = next) {
				next = ht->next;
				if (!(table->flag & KEY_REUSE))
					acl_myfree(ht->key.key);
				if (free_fn)
					(*free_fn) (ht->value);
				if (table->slice)
					acl_slice_free2(table->slice, ht);
				else
					acl_myfree(ht);
			}
		}
		acl_myfree(table->data);
		if (table->slice)
			acl_slice_destroy(table->slice);
		acl_myfree(table);
	}
}

/* binhash_walk - iterate over hash table */

void    binhash_walk(BINHASH *table, void (*action) (BINHASH_INFO *, char *),
		char *ptr) {
	if (table != 0) {
		unsigned i = table->size;
		BINHASH_INFO **h = table->data;
		BINHASH_INFO *ht;

		while (i-- > 0)
			for (ht = *h++; ht; ht = ht->next)
				(*action) (ht, ptr);
	}
}

/* binhash_list - list all table members */

BINHASH_INFO **binhash_list(BINHASH *table)
{
	BINHASH_INFO **list;
	BINHASH_INFO *member;
	int     count = 0;
	int     i;

	if (table != 0) {
		list = (BINHASH_INFO **) acl_mymalloc(sizeof(*list) * (table->used + 1));
		for (i = 0; i < table->size; i++)
			for (member = table->data[i]; member != 0; member = member->next)
				list[count++] = member;
	} else {
		list = (BINHASH_INFO **) acl_mymalloc(sizeof(*list));
	}
	list[count] = 0;
	return (list);
}

