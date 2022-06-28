#include "stdlib/acl_define.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_slice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "htable.h"

/* Structure of one hash table. */

struct HTABLE {
	int     size;			/* length of entries array */
	int     init_size;		/* length of initial entryies array */
	int     used;			/* number of entries in table */
	HTABLE_INFO **data;		/* entries array, auto-resized */
	int     status;			/* the operator's status on the htable */

	ACL_SLICE *slice;
	unsigned int flag;		/* same as ACL_MDT_IDX->flag */
	HASH_FN  hash_fn;		/* hash function */
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

static int htable_size(HTABLE *table, unsigned size)
{
	HTABLE_INFO **h;

	size |= 1;

	table->data = h = (HTABLE_INFO **) acl_mymalloc(size * sizeof(HTABLE_INFO *));
	if(table->data == NULL)
		return(-1);

	table->size = size;
	table->used = 0;

	while (size-- > 0)
		*h++ = 0;

	return(0);
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

	ret = htable_size(table, 2 * old_size);
	if (ret < 0)
		return(-1);

	while (old_size-- > 0) {
		for (ht = *h0++; ht; ht = next) {
			next = ht->next;
			n = table->hash_fn(ht->key.c_key, strlen(ht->key.c_key)) % table->size;
			htable_link(table, ht, n);
		}
	}

	acl_myfree(old_entries);

	return(0);
}

/* htable_create - create initial hash table */

HTABLE *htable_create(int size, unsigned int flag, int slice_type)
{
	HTABLE *table;
	int	ret;

	table = (HTABLE *) acl_mycalloc(1, sizeof(HTABLE));
	if(table == NULL)
		return(NULL);

	table->init_size = size;

	ret = htable_size(table, size < 13 ? 13 : size);
	if(ret < 0) {
		acl_myfree(table);
		return(NULL);
	}

	if (slice_type)
		table->slice = acl_slice_create("htable", 0,
					sizeof(HTABLE_INFO), slice_type);

	table->hash_fn = __def_hash_fn;
	table->flag = flag;
	return (table);
}

void htable_ctl(HTABLE *table, int name, ...)
{
	const char *myname = "htable_ctl";
	va_list ap;

	if (table == NULL)
		return;

	va_start(ap, name);
	for (; name != HTABLE_CTL_END; name = va_arg(ap, int)) {
		switch (name) {
		case HTABLE_CTL_HASH_FN:
			table->hash_fn = va_arg(ap, HASH_FN);
			if (table->hash_fn == NULL)
				table->hash_fn = __def_hash_fn;
			break;
		default:
			acl_msg_fatal("%s: bad name %d", myname, name);
		}
	}
	va_end(ap);
}

int htable_last_errno(HTABLE *table)
{
	if (table == NULL)
		return (HTABLE_STAT_INVAL);
	return (table->status);
}

void htable_set_errno(HTABLE *table, int error)
{
	if (table)
		table->status = error;
}

#define	STREQ(x,y) (x == y || (x[0] == y[0] && strcmp(x,y) == 0))

/* htable_enter - enter (key, value) pair */

HTABLE_INFO *htable_enter(HTABLE *table, const char *key, char *value acl_unused)
{
	const char *myname = "htable_enter";
	HTABLE_INFO *ht;
	int   ret;
	unsigned n;

	table->status = HTABLE_STAT_OK;

	if (table->used >= table->size) {
		ret = htable_grow(table);
		if(ret < 0) {
			return(NULL);
		}
	}

	n = table->hash_fn(key, strlen(key));
	n = n % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key.c_key)) {
			table->status = HTABLE_STAT_DUPLEX_KEY;
			return (ht);
		}
	}

	if (table->slice)
		ht = (HTABLE_INFO*) acl_slice_alloc(table->slice);
	else
		ht = (HTABLE_INFO *) acl_mymalloc(sizeof(HTABLE_INFO));
	if (ht == NULL) {
		acl_msg_error("%s(%d): alloc error", myname, __LINE__);
		return(NULL);
	}

	if (table->flag & KEY_REUSE)
		ht->key.c_key = key;
	else
		ht->key.key = acl_mystrdup(key);
	if (ht->key.key == NULL) {
		acl_myfree(ht);
		return(NULL);
	}

#ifdef	HAS_VALUE
	ht->value = value;
#endif
	htable_link(table, ht, n);

	return (ht);
}

/* htable_find - lookup value */

char *htable_find(HTABLE *table, const char *key)
{
	HTABLE_INFO *ht;
	unsigned  n;

#ifndef	HAS_VALUE
	return (NULL);
#endif
	if (table == NULL || key == NULL)
		return (NULL);

	n = table->hash_fn(key, strlen(key));

	n = n % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key.c_key)) {
#ifdef	HAS_VALUE
			return (ht->value);
#endif
		}
	}

	return (NULL);
}

/* htable_locate - lookup entry */

HTABLE_INFO *htable_locate(HTABLE *table, const char *key)
{
	HTABLE_INFO *ht;
	unsigned  n;

	if(table == NULL || key == NULL)
		return (NULL);

	n = table->hash_fn(key, strlen(key));

	n = n % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key.c_key)) {
			return (ht);
		}
	}

	return (NULL);
}

/* htable_delete - delete one entry */

int htable_delete(HTABLE *table, const char *key, void (*free_fn) (char *) acl_unused)
{
	if (table && key) {
		HTABLE_INFO *ht;
		unsigned  n = table->hash_fn(key, strlen(key));
		HTABLE_INFO **h;

		n = n % table->size;

		h = table->data + n;
		for (ht = *h; ht; ht = ht->next) {
			if (STREQ(key, ht->key.c_key)) {
				if (ht->next)
					ht->next->prev = ht->prev;
				if (ht->prev)
					ht->prev->next = ht->next;
				else
					*h = ht->next;
				table->used--;
#ifdef	HAS_VALUE
				if (free_fn && ht->value)
					(*free_fn) (ht->value);
#endif
				if (!(table->flag & KEY_REUSE))
					acl_myfree(ht->key.key);
				if (table->slice)
					acl_slice_free2(table->slice, ht);
				else
					acl_myfree(ht);
				return(0);
			}
		}
	}

	return(-1);
}

/* htable_free - destroy hash table */

void htable_free(HTABLE *table, void (*free_fn) (char *) acl_unused)
{
	if (table) {
		unsigned i = table->size;
		HTABLE_INFO *ht;
		HTABLE_INFO *next;
		HTABLE_INFO **h = table->data;

		while (i-- > 0) {
			for (ht = *h++; ht; ht = next) {
				next = ht->next;
#ifdef	HAS_VALUE
				if (free_fn && ht->value)
					(*free_fn) (ht->value);
#endif
				if (!(table->flag & KEY_REUSE))
					acl_myfree(ht->key.key);
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

/* htable_walk - iterate over hash table */

void htable_walk(HTABLE *table, void (*action)(HTABLE_INFO *, char *), char *ptr)
{
	if (table) {
		unsigned i = table->size;
		HTABLE_INFO **h = table->data;
		HTABLE_INFO *ht;

		while (i-- > 0)
			for (ht = *h++; ht; ht = ht->next)
				(*action) (ht, ptr);
	}
}

int htable_capacity(const HTABLE *table)
{
	if (table)
		return (table->size);
	else
		return (0);
}

int htable_used(const HTABLE *table)
{
	if (table)
		return (table->used);
	else
		return (0);
}

/* htable_list - list all table members */

HTABLE_INFO **htable_list(const HTABLE *table)
{
	HTABLE_INFO **list;
	HTABLE_INFO *member;
	int     count = 0;
	int     i;

	if (table != 0) {
		list = (HTABLE_INFO **) acl_mymalloc(sizeof(*list) * (table->used + 1));
		for (i = 0; i < table->size; i++)
			for (member = table->data[i]; member != 0; member = member->next)
				list[count++] = member;
	} else {
		list = (HTABLE_INFO **) acl_mymalloc(sizeof(*list));
	}
	list[count] = 0;
	return (list);
}

void htable_list_free(HTABLE_INFO **list)
{
	acl_myfree(list);
}

void htable_stat(const HTABLE *table)
{
	HTABLE_INFO *member;
	int	i, count;

	if(table != NULL) {
		printf("hash stat count for each key:\n");
		for(i = 0; i < table->size; i++) {
			count = 0;
			member = table->data[i];
			for(; member != 0; member = member->next)
				count++;
			if(count > 0)
				printf("chains[%d]: count[%d]\n", i, count);
		}

		printf("hash stat all values for each key:\n");
		for(i = 0; i < table->size; i++) {
			member = table->data[i];
			if(member) {
				printf("chains[%d]: ", i);
				for(; member != 0; member = member->next)
					printf("[%s]", member->key.c_key);
				printf("\n");
			}
		}
		printf("hash table size=%d, used=%d\n", table->size, table->used);
	}
}
