#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#ifdef ACL_WINDOWS
# define SANE_STRDUP _strdup
#else
# define SANE_STRDUP strdup
#endif

#define MALLOC	malloc
#define CALLOC	calloc
#define REALLOC	realloc
#define FREE	free

#include "htable.h"

/* Structure of one hash table. */

struct DEBUG_HTABLE {
	int     size;			/* length of entries array */
	int     init_size;		/* length of initial entryies array */
	int     used;			/* number of entries in table */
	DEBUG_HTABLE_INFO **data;		/* entries array, auto-resized */
	DEBUG_HASH_FN  hash_fn;		/* hash function */
};

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
/* debug_htable_link - insert element into table */

#define debug_htable_link(_table, _element, _n) { \
	DEBUG_HTABLE_INFO **_h = _table->data + _n; \
	_element->prev = 0; \
	if ((_element->next = *_h) != 0) \
		(*_h)->prev = _element; \
	*_h = _element; \
	_table->used++; \
}

/* debug_htable_size - allocate and initialize hash table */

static int debug_htable_size(DEBUG_HTABLE *table, unsigned size)
{
	DEBUG_HTABLE_INFO **h;

	size |= 1;

	table->data = h = (DEBUG_HTABLE_INFO **) MALLOC(size * sizeof(DEBUG_HTABLE_INFO *));
	if(table->data == NULL)
		return(-1);

	table->size = size;
	table->used = 0;

	while (size-- > 0)
		*h++ = 0;

	return(0);
}

/* debug_htable_grow - extend existing table */

static int debug_htable_grow(DEBUG_HTABLE *table)
{
	int ret;
	DEBUG_HTABLE_INFO *ht;
	DEBUG_HTABLE_INFO *next;
	unsigned old_size = table->size;
	DEBUG_HTABLE_INFO **h0 = table->data;
	DEBUG_HTABLE_INFO **old_entries = h0;
	unsigned n;

	ret = debug_htable_size(table, 2 * old_size);
	if (ret < 0)
		return(-1);

	while (old_size-- > 0) {
		for (ht = *h0++; ht; ht = next) {
			next = ht->next;
			n = table->hash_fn(ht->key, strlen(ht->key)) % table->size;
			debug_htable_link(table, ht, n);
		}
	}

	FREE(old_entries);

	return(0);
}

/* debug_htable_create - create initial hash table */

DEBUG_HTABLE *debug_htable_create(int size)
{
	DEBUG_HTABLE *table;
	int	ret;

	table = (DEBUG_HTABLE *) CALLOC(1, sizeof(DEBUG_HTABLE));
	if(table == NULL)
		return (NULL);

	table->init_size = size;

	ret = debug_htable_size(table, size < 13 ? 13 : size);
	if(ret < 0) {
		FREE(table);
		return(NULL);
	}

	table->hash_fn = __def_hash_fn;
	return (table);
}

#define	STREQ(x,y) (x == y || (x[0] == y[0] && strcmp(x,y) == 0))

/* debug_htable_enter - enter (key, value) pair */

DEBUG_HTABLE_INFO *debug_htable_enter(DEBUG_HTABLE *table, const char *key, char *value)
{
	DEBUG_HTABLE_INFO *ht;
	int   ret;
	unsigned n;

	n = table->hash_fn(key, strlen(key));

	if (table->used >= table->size) {
		ret = debug_htable_grow(table);
		if(ret < 0) {
			return(NULL);
		}
	}

	n = n % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key)) {
			return (ht);
		}
	}

	ht = (DEBUG_HTABLE_INFO *) MALLOC(sizeof(DEBUG_HTABLE_INFO));
	if (ht == NULL) {
		return(NULL);
	}

	ht->key = SANE_STRDUP(key);
	if (ht->key == NULL) {
		FREE(ht);
		return(NULL);
	}

	ht->value = value;
	debug_htable_link(table, ht, n);

	return (ht);
}

/* debug_htable_find - lookup value */

char *debug_htable_find(DEBUG_HTABLE *table, const char *key)
{
	DEBUG_HTABLE_INFO *ht;
	unsigned  n;

	if (table == NULL || key == NULL)
		return (NULL);

	n = table->hash_fn(key, strlen(key));

	n = n % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key)) {
			return (ht->value);
		}
	}

	return (NULL);
}

/* debug_htable_locate - lookup entry */

DEBUG_HTABLE_INFO *debug_htable_locate(DEBUG_HTABLE *table, const char *key)
{
	DEBUG_HTABLE_INFO *ht;
	unsigned  n;

	if(table == NULL || key == NULL)
		return (NULL);

	n = table->hash_fn(key, strlen(key));

	n = n % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key)) {
			return (ht);
		}
	}

	return (NULL);
}

/* debug_htable_delete - delete one entry */

int debug_htable_delete(DEBUG_HTABLE *table, const char *key, void (*free_fn) (char *))
{
	if (table && key) {
		DEBUG_HTABLE_INFO *ht;
		unsigned  n = table->hash_fn(key, strlen(key));
		DEBUG_HTABLE_INFO **h;
		
		n = n % table->size;

		h = table->data + n;
		for (ht = *h; ht; ht = ht->next) {
			if (STREQ(key, ht->key)) {
				if (ht->next)
					ht->next->prev = ht->prev;
				if (ht->prev)
					ht->prev->next = ht->next;
				else
					*h = ht->next;
				table->used--;
				FREE(ht->key);
				if (free_fn && ht->value)
					(*free_fn) (ht->value);
				FREE(ht);
				return(0);
			}
		}
	}

	return(-1);
}

/* debug_htable_free - destroy hash table */

void debug_htable_free(DEBUG_HTABLE *table, void (*free_fn) (char *))
{
	if (table) {
		unsigned i = table->size;
		DEBUG_HTABLE_INFO *ht;
		DEBUG_HTABLE_INFO *next;
		DEBUG_HTABLE_INFO **h = table->data;

		while (i-- > 0) {
			for (ht = *h++; ht; ht = next) {
				next = ht->next;
				FREE(ht->key);
				if (free_fn && ht->value)
					(*free_fn) (ht->value);
				FREE(ht);
			}
		}
		FREE(table->data);
		table->data = 0;
		FREE(table);
	}
}

int debug_htable_reset(DEBUG_HTABLE *table, void (*free_fn) (char *))
{
	if (table) {
		unsigned i = table->size;
		DEBUG_HTABLE_INFO *ht;
		DEBUG_HTABLE_INFO *next;
		DEBUG_HTABLE_INFO **h;
		int ret;

		h = table->data;

		while (i-- > 0) {
			for (ht = *h++; ht; ht = next) {
				next = ht->next;
				FREE(ht->key);
				if (free_fn && ht->value)
					(*free_fn) (ht->value);
				FREE(ht);
			}
		}
		FREE(table->data);
		table->data = 0;

		ret = debug_htable_size(table, table->init_size < 13 ? 13 : table->init_size);
		return (ret);
	}

	return (-1);
}

/* debug_htable_walk - iterate over hash table */

void debug_htable_walk(DEBUG_HTABLE *table, void (*action)(DEBUG_HTABLE_INFO *, char *), char *ptr)
{
	if (table) {
		unsigned i = table->size;
		DEBUG_HTABLE_INFO **h = table->data;
		DEBUG_HTABLE_INFO *ht;

		while (i-- > 0)
			for (ht = *h++; ht; ht = ht->next)
				(*action) (ht, ptr);
	}
}

/* debug_htable_list - list all table members */

DEBUG_HTABLE_INFO **debug_htable_list(const DEBUG_HTABLE *table)
{
	DEBUG_HTABLE_INFO **list;
	DEBUG_HTABLE_INFO *member;
	int     count = 0;
	int     i;

	if (table != 0) {
		list = (DEBUG_HTABLE_INFO **) MALLOC(sizeof(*list) * (table->used + 1));
		for (i = 0; i < table->size; i++)
			for (member = table->data[i]; member != 0; member = member->next)
				list[count++] = member;
	} else {
		list = (DEBUG_HTABLE_INFO **) MALLOC(sizeof(*list));
	}
	list[count] = 0;
	return (list);
}
