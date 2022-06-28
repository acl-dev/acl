#include "stdafx.h"
#include "memory.h"
#include "msg.h"
#include "htable.h"

/* htable_iter_head */

static void *htable_iter_head(ITER *iter, HTABLE *table)
{
	HTABLE_INFO *ptr = NULL;

	iter->dlen = -1;
	iter->klen = -1;
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
		iter->key = ptr->key;
	} else {
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

/* htable_iter_next */

static void *htable_iter_next(ITER *iter, HTABLE *table)
{
	HTABLE_INFO *ptr;

	ptr = (HTABLE_INFO*) iter->ptr;
	if (ptr) {
		iter->ptr = ptr = ptr->next;
		if (ptr != NULL) {
			iter->data = ptr->value;
			iter->key = ptr->key;
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
		iter->key = ptr->key;
	} else {
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

/* htable_iter_tail */

static void *htable_iter_tail(ITER *iter, HTABLE *table)
{
	HTABLE_INFO *ptr = NULL;

	iter->dlen = -1;
	iter->klen = -1;
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
		iter->key = ptr->key;
	} else {
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

/* htable_iter_prev */

static void *htable_iter_prev(ITER *iter, HTABLE *table)
{
	HTABLE_INFO *ptr;

	ptr = (HTABLE_INFO*) iter->ptr;
	if (ptr) {
		iter->ptr = ptr = ptr->next;
		if (ptr != NULL) {
			iter->data = ptr->value;
			iter->key = ptr->key;
			return (iter->ptr);
		}
	}

	for (iter->i--; iter->i >= 0; iter->i--) {
		if (table->data[iter->i] != NULL) {
			iter->ptr = ptr = table->data[iter->i];
			break;
		}
	}

	if (ptr) {
		iter->data = ptr->value;
		iter->key = ptr->key;
	} else {
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

/* htable_iter_info */

static HTABLE_INFO *htable_iter_info(ITER *iter, struct HTABLE *table)
{
	(void) table;
	return (iter->ptr ? (HTABLE_INFO*) iter->ptr : NULL);
}

/* __def_hash_fn - hash a string */

static unsigned __def_hash_fn(const void *buffer, size_t len)
{
        unsigned long h = 0;
        unsigned long g;
	const unsigned char* s = (const unsigned char *) buffer;

        /*
         * From the "Dragon" book by Aho, Sethi and Ullman.
         */

        while (len-- > 0) {
                h = (h << 4) + *s++;
                if ((g = (h & 0xf0000000)) != 0) {
                        h ^= (g >> 24);
                        h ^= g;
                }
        }

        return (unsigned) h;
}
/* htable_link - insert element into table */

#define htable_link(_table, _element, _n) { \
	HTABLE_INFO **_h = _table->data + _n; \
	_element->prev = 0; \
	if ((_element->next = *_h) != 0) \
		(*_h)->prev = _element; \
	*_h = _element; \
	_table->used++; \
}

/* htable_size - allocate and initialize hash table */

static int __htable_size(HTABLE *table, unsigned size)
{
	HTABLE_INFO **h;

	size |= 1;

	table->data = h = (HTABLE_INFO **) mem_malloc(size * sizeof(HTABLE_INFO *));
	if(table->data == NULL) {
		return -1;
	}

	table->size = size;
	table->used = 0;

	while (size-- > 0) {
		*h++ = 0;
	}

	return 0;
}

/* htable_grow - extend existing table */

static int htable_grow(HTABLE *table)
{
	int ret;
	HTABLE_INFO *ht;
	HTABLE_INFO *next;
	unsigned old_size = table->size;
	HTABLE_INFO **h0 = table->data;
	HTABLE_INFO **old_entries = h0;
	unsigned n;

	ret = __htable_size(table, 2 * old_size);
	if (ret < 0) {
		return -1;
	}

	while (old_size-- > 0) {
		for (ht = *h0++; ht; ht = next) {
			next = ht->next;
			n = __def_hash_fn(ht->key, strlen(ht->key)) % table->size;
			htable_link(table, ht, n);
		}
	}

	mem_free(old_entries);
	return 0;
}

HTABLE *htable_create(int size)
{
	HTABLE *table;
	int	ret;

	table =	(HTABLE *) mem_calloc(1, sizeof(HTABLE));
	if (table == NULL) {
		return NULL;
	}

	table->init_size = size;
	ret = __htable_size(table, size < 13 ? 13 : size);
	if(ret < 0) {
		mem_free(table);
		return NULL;
	}

	table->iter_head = htable_iter_head;
	table->iter_next = htable_iter_next;
	table->iter_tail = htable_iter_tail;
	table->iter_prev = htable_iter_prev;
	table->iter_info = htable_iter_info;

	return table;
}

int htable_errno(HTABLE *table)
{
	if (table == NULL) {
		return HTABLE_STAT_INVAL;
	}
	return table->status;
}

void htable_set_errno(HTABLE *table, int error)
{
	if (table) {
		table->status = error;
	}
}

#define	STREQ(x,y) (x == y || (x[0] == y[0] && strcmp(x,y) == 0))

/* htable_enter - enter (key, value) pair */

HTABLE_INFO *htable_enter(HTABLE *table, const char *key, void *value)
{
	HTABLE_INFO *ht;
	int   ret;
	unsigned hash, n;

	table->status = HTABLE_STAT_OK;
	hash = __def_hash_fn(key, strlen(key));

	if (table->used >= table->size) {
		ret = htable_grow(table);
		if(ret < 0) {
			return NULL;
		}
	}

	n = hash % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key)) {
			table->status = HTABLE_STAT_DUPLEX_KEY;
			msg_info("%s(%d): duplex key(%s) exist",
				__FUNCTION__, __LINE__, key);
			return ht;
		}
	}

	ht = (HTABLE_INFO *) mem_malloc(sizeof(HTABLE_INFO));
	if (ht == NULL) {
		msg_error("%s(%d): alloc error", __FUNCTION__, __LINE__);
		return NULL;
	}

#if defined(_WIN32) || defined(_WIN64)
	ht->key = _strdup(key);
#else
	ht->key = mem_strdup(key);
#endif
	if (ht->key == NULL) {
		msg_error("%s(%d): alloc error", __FUNCTION__, __LINE__);
		mem_free(ht);
		return NULL;
	}
	ht->hash  = hash;
	ht->value = value;
	htable_link(table, ht, n);

	return ht;
}

/* htable_find - lookup value */

void *htable_find(HTABLE *table, const char *key)
{
	HTABLE_INFO *ht = htable_locate(table, key);

	return ht != NULL ? ht->value : NULL;
}

/* htable_locate - lookup entry */

HTABLE_INFO *htable_locate(HTABLE *table, const char *key)
{
	HTABLE_INFO *ht;
	unsigned     n;

	n = __def_hash_fn(key, strlen(key));
	n = n % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key)) {
			return ht;
		}
	}

	return NULL;
}

void htable_delete_entry(HTABLE *table, HTABLE_INFO *ht,
	void (*free_fn) (void *))
{
	unsigned n = ht->hash % table->size;
	HTABLE_INFO **h = table->data + n;

	if (ht->next)
		ht->next->prev = ht->prev;
	if (ht->prev)
		ht->prev->next = ht->next;
	else
		*h = ht->next;

	mem_free(ht->key);
	if (free_fn && ht->value)
		(*free_fn) (ht->value);
	mem_free(ht);
	table->used--;
}

/* htable_delete - delete one entry */

int htable_delete(HTABLE *table, const char *key, void (*free_fn) (void *))
{
	HTABLE_INFO *ht;
	unsigned     n;
	HTABLE_INFO **h;

	n = __def_hash_fn(key, strlen(key));
	n = n % table->size;

	h = table->data + n;
	for (ht = *h; ht; ht = ht->next) {
		if (STREQ(key, ht->key)) {
			htable_delete_entry(table, ht, free_fn);
			return 0;
		}
	}
	return -1;
}

/* htable_free - destroy hash table */

void htable_free(HTABLE *table, void (*free_fn) (void *))
{
	unsigned i = table->size;
	HTABLE_INFO  *ht;
	HTABLE_INFO  *next;
	HTABLE_INFO **h = table->data;

	while (i-- > 0) {
		for (ht = *h++; ht; ht = next) {
			next = ht->next;
			mem_free(ht->key);
			if (free_fn && ht->value)
				(*free_fn) (ht->value);
			mem_free(ht);
		}
	}

	mem_free(table->data);
	table->data = 0;
	mem_free(table);
}

int htable_reset(HTABLE *table, void (*free_fn) (void *))
{
	unsigned i = table->size;
	HTABLE_INFO *ht;
	HTABLE_INFO *next;
	HTABLE_INFO **h;
	int ret;

	h = table->data;

	while (i-- > 0) {
		for (ht = *h++; ht; ht = next) {
			next = ht->next;
			mem_free(ht->key);
			if (free_fn && ht->value) {
				(*free_fn) (ht->value);
			}
			mem_free(ht);
		}
	}
	mem_free(table->data);
	ret = __htable_size(table, table->init_size < 13 ? 13 : table->init_size);
	return ret;
}

/* htable_walk - iterate over hash table */

void htable_walk(HTABLE *table, void (*action)(HTABLE_INFO *, void *), void *arg)
{
	unsigned i = table->size;
	HTABLE_INFO **h = table->data;
	HTABLE_INFO *ht;

	while (i-- > 0) {
		for (ht = *h++; ht; ht = ht->next) {
			(*action) (ht, arg);
		}
	}
}

int htable_size(const HTABLE *table)
{
	if (table) {
		return table->size;
	} else {
		return 0;
	}
}

int htable_used(const HTABLE *table)
{
	if (table) {
		return table->used;
	} else {
		return (0);
	}
}

/*
HTABLE_INFO **htable_data(HTABLE *table)
{
	return (HTABLE_INFO**) table->data;
}
*/

/* htable_list - list all table members */

HTABLE_INFO **htable_list(const HTABLE *table)
{
	HTABLE_INFO **list;
	HTABLE_INFO *member;
	int     count = 0;
	int     i;

	if (table != 0) {
		list = (HTABLE_INFO **) mem_malloc(sizeof(*list) * (table->used + 1));
		for (i = 0; i < table->size; i++) {
			for (member = table->data[i]; member != 0;
				member = member->next) {
				list[count++] = member;
			}
		}
	} else {
		list = (HTABLE_INFO **) mem_malloc(sizeof(*list));
	}
	list[count] = 0;
	return list;
}

void htable_stat(const HTABLE *table)
{
	HTABLE_INFO *member;
	int	i, count;

	printf("hash stat count for each key:\n");
	for(i = 0; i < table->size; i++) {
		count = 0;
		member = table->data[i];
		for(; member != 0; member = member->next) {
			count++;
		}
		if(count > 0) {
			printf("chains[%d]: count[%d]\n", i, count);
		}
	}

	printf("hash stat all values for each key:\n");
	for(i = 0; i < table->size; i++) {
		member = table->data[i];
		if(member) {
			printf("chains[%d]: ", i);
			for(; member != 0; member = member->next)
				printf("[%s]", member->key);
			printf("\n");
		}
	}
	printf("hash table size=%d, used=%d\n", table->size, table->used);
}
