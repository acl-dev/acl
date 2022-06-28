#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "thread/acl_pthread.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_slice.h"
#include "stdlib/acl_iterator.h"
#include "stdlib/acl_htable.h"
#include "stdlib/acl_mystring.h"

#endif

/* htable_iter_head */

static void *htable_iter_head(ACL_ITER *iter, ACL_HTABLE *table)
{
	ACL_HTABLE_INFO *ptr = NULL;

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
		iter->key = ptr->key.c_key;
	} else {
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

/* htable_iter_next */

static void *htable_iter_next(ACL_ITER *iter, ACL_HTABLE *table)
{
	ACL_HTABLE_INFO *ptr;

	ptr = (ACL_HTABLE_INFO*) iter->ptr;
	if (ptr) {
		iter->ptr = ptr = ptr->next;
		if (ptr != NULL) {
			iter->data = ptr->value;
			iter->key = ptr->key.c_key;
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
		iter->key = ptr->key.c_key;
	} else {
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

/* htable_iter_tail */

static void *htable_iter_tail(ACL_ITER *iter, ACL_HTABLE *table)
{
	ACL_HTABLE_INFO *ptr = NULL;

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
		iter->key = ptr->key.c_key;
	} else {
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

/* htable_iter_prev */

static void *htable_iter_prev(ACL_ITER *iter, ACL_HTABLE *table)
{
	ACL_HTABLE_INFO *ptr;

	ptr = (ACL_HTABLE_INFO*) iter->ptr;
	if (ptr) {
		iter->ptr = ptr = ptr->next;
		if (ptr != NULL) {
			iter->data = ptr->value;
			iter->key = ptr->key.c_key;
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
		iter->key = ptr->key.c_key;
	} else {
		iter->data = NULL;
		iter->key = NULL;
	}
	return (iter->ptr);
}

/* htable_iter_info */

static ACL_HTABLE_INFO *htable_iter_info(ACL_ITER *iter, struct ACL_HTABLE *table acl_unused)
{
	return (iter->ptr ? (ACL_HTABLE_INFO*) iter->ptr : NULL);
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
	ACL_HTABLE_INFO **_h = _table->data + _n; \
	_element->prev = 0; \
	if ((_element->next = *_h) != 0) \
		(*_h)->prev = _element; \
	*_h = _element; \
	_table->used++; \
}

/* htable_size - allocate and initialize hash table */

static int htable_size(ACL_HTABLE *table, unsigned size)
{
	ACL_HTABLE_INFO **h;

	size |= 1;

	if (table->slice)
		table->data = h = (ACL_HTABLE_INFO **)
			acl_slice_pool_alloc(__FILE__, __LINE__, table->slice,
					size * sizeof(ACL_HTABLE_INFO *));
	else
		table->data = h = (ACL_HTABLE_INFO **)
			acl_mymalloc(size * sizeof(ACL_HTABLE_INFO *));
	if(table->data == NULL)
		return(-1);

	table->size = size;
	table->used = 0;

	while (size-- > 0)
		*h++ = 0;

	return(0);
}

/* htable_grow - extend existing table */

static int htable_grow(ACL_HTABLE *table)
{
	int ret;
	ACL_HTABLE_INFO *ht;
	ACL_HTABLE_INFO *next;
	unsigned old_size = table->size;
	ACL_HTABLE_INFO **h0 = table->data;
	ACL_HTABLE_INFO **old_entries = h0;
	unsigned n;

	ret = htable_size(table, 2 * old_size);
	if (ret < 0)
		return(-1);

	while (old_size-- > 0) {
		for (ht = *h0++; ht; ht = next) {
			next = ht->next;
			n = table->hash_fn(ht->key.c_key,
				strlen(ht->key.c_key)) % table->size;
			htable_link(table, ht, n);
		}
	}

	if (table->slice)
		acl_slice_pool_free(__FILE__, __LINE__, old_entries);
	else
		acl_myfree(old_entries);
	return(0);
}

#define	_RWLOCK_TYPE	acl_pthread_mutex_t
#define	_RWLOCK_INIT	acl_pthread_mutex_init
#define	_RWLOCK_DESTROY	acl_pthread_mutex_destroy
#define	_RWLOCK_RDLOCK	acl_pthread_mutex_lock
#define	_RWLOCK_WRLOCK	acl_pthread_mutex_lock
#define	_RWLOCK_UNLOCK	acl_pthread_mutex_unlock

static int __init_table_rwlock(ACL_HTABLE *table, int enable)
{
	const char *myname = "__init_table_rwlock";
	char  tbuf[256];
	int   ret;

	if (enable && table->rwlock == NULL) {
		if (table->slice)
			table->rwlock = acl_slice_pool_calloc(__FILE__, __LINE__,
						table->slice, 1, sizeof(_RWLOCK_TYPE));
		else
			table->rwlock = acl_mycalloc(1, sizeof(_RWLOCK_TYPE));
		if (table->rwlock == NULL) {
			acl_msg_error("%s(%s): calloc error(%s)",
				__FILE__, myname, acl_last_strerror(tbuf, sizeof(tbuf)));
			return (-1);
		}

		ret = _RWLOCK_INIT(table->rwlock, NULL);
		if (ret) {
			acl_msg_error("%s(%d): init rwlock error(%s)",
				__FILE__, __LINE__, acl_strerror(ret, tbuf, sizeof(tbuf)));
			if (table->slice)
				acl_slice_pool_free(__FILE__, __LINE__, table->rwlock);
			else
				acl_myfree(table->rwlock);
			return (-1);
		}
	} else if (!enable && table->rwlock) {
		_RWLOCK_DESTROY(table->rwlock);
		if (table->slice)
			acl_slice_pool_free(__FILE__, __LINE__, table->rwlock);
		else
			acl_myfree(table->rwlock);
		table->rwlock = NULL;
	}

	return (0);
}

/* acl_htable_create - create initial hash table */

ACL_HTABLE *acl_htable_create(int size, unsigned int flag)
{
	return (acl_htable_create3(size, flag, NULL));
}

ACL_HTABLE *acl_htable_create3(int size, unsigned int flag, ACL_SLICE_POOL *slice)
{
	ACL_HTABLE *table;
	int	ret;

	if (slice) {
		table = (ACL_HTABLE *) acl_slice_pool_calloc(__FILE__, __LINE__,
				slice, 1, sizeof(ACL_HTABLE));
		if (table == NULL)
			return (NULL);
		table->slice = slice;
	} else {
		table =	(ACL_HTABLE *) acl_mycalloc(1, sizeof(ACL_HTABLE));
		if (table == NULL)
			return (NULL);
		table->slice = NULL;
	}

	table->init_size = size;
	table->flag = flag;

	ret = htable_size(table, size < 13 ? 13 : size);
	if(ret < 0) {
		if (table->slice)
			acl_slice_pool_free(__FILE__, __LINE__, table);
		else
			acl_myfree(table);
		return(NULL);
	}

	table->hash_fn = __def_hash_fn;

	table->iter_head = htable_iter_head;
	table->iter_next = htable_iter_next;
	table->iter_tail = htable_iter_tail;
	table->iter_prev = htable_iter_prev;
	table->iter_info = htable_iter_info;

	if ((flag & ACL_HTABLE_FLAG_USE_LOCK)) {
		ret = __init_table_rwlock(table, 1);
		if (ret < 0) {
			if (table->slice)
				acl_slice_pool_free(__FILE__, __LINE__, table);
			else
				acl_myfree(table);
			return (NULL);
		}
	} else
		table->rwlock = NULL;

	return (table);
}

#ifdef ACL_BCB_COMPILER
static void LOCK_TABLE_READ(const ACL_HTABLE *table)
{
	if (table->rwlock && _RWLOCK_RDLOCK(table->rwlock) != 0) {
		acl_msg_fatal("%s(%d): read lock error",
			__FILE__, __LINE__);
	}
}

static void LOCK_TABLE_WRITE(const ACL_HTABLE *table)
{
	if (table->rwlock && _RWLOCK_WRLOCK(table->rwlock) != 0) {
		acl_msg_fatal("%s(%d): write lock error",
			__FILE__, __LINE__);
	}
}

static void UNLOCK_TABLE(const ACL_HTABLE *table)
{
	if (table->rwlock && _RWLOCK_UNLOCK(table->rwlock) != 0) {
		acl_msg_fatal("%s(%d): unlock error",
			__FILE__, __LINE__);
	}
}
#else
#define LOCK_TABLE_READ(_table) do { \
	int   _ret; \
	if (_table->rwlock && (_ret = _RWLOCK_RDLOCK(_table->rwlock))) { \
		acl_msg_fatal("%s(%d): read lock error(%s)", \
				__FILE__, __LINE__, strerror(_ret)); \
	} \
} while (0)

#define	LOCK_TABLE_WRITE(_table) do { \
	int   _ret; \
	if (_table->rwlock && (_ret = _RWLOCK_WRLOCK(_table->rwlock))) { \
		acl_msg_fatal("%s(%d): write lock error(%s)", \
				__FILE__, __LINE__, strerror(_ret)); \
	} \
} while (0)

#define	UNLOCK_TABLE(_table) do { \
	int   _ret; \
	if (_table->rwlock && (_ret = _RWLOCK_UNLOCK(_table->rwlock))) { \
		acl_msg_fatal("%s(%d): unlock error(%s)", \
				__FILE__, __LINE__, strerror(_ret)); \
	} \
} while (0)
#endif

void acl_htable_ctl(ACL_HTABLE *table, int name, ...)
{
	const char *myname = "acl_htable_ctl";
	va_list ap;

	if (table == NULL)
		return;

	va_start(ap, name);
	for (; name != ACL_HTABLE_CTL_END; name = va_arg(ap, int)) {
		switch (name) {
		case ACL_HTABLE_CTL_HASH_FN:
			table->hash_fn = va_arg(ap, ACL_HASH_FN);
			if (table->hash_fn == NULL)
				table->hash_fn = __def_hash_fn;
			break;
		case ACL_HTABLE_CTL_RWLOCK:
			if (__init_table_rwlock(table, va_arg(ap, int)) < 0)
				acl_msg_fatal("%s: init rwlock error", myname);

			break;
		default:
			acl_msg_fatal("%s: bad name %d", myname, name);
		}
	}
	va_end(ap);
}

int acl_htable_errno(ACL_HTABLE *table)
{
	if (table == NULL)
		return (ACL_HTABLE_STAT_INVAL);
	return (table->status);
}

void acl_htable_set_errno(ACL_HTABLE *table, int error)
{
	if (table)
		table->status = error;
}

#define	STREQ(x,y) (x == y || (x[0] == y[0] && strcmp(x,y) == 0))

/* acl_htable_enter - enter (key, value) pair */

ACL_HTABLE_INFO *acl_htable_enter(ACL_HTABLE *table, const char *key_in, void *value)
{
	const char *myname = "acl_htable_enter";
	ACL_HTABLE_INFO *ht;
	int   ret;
	unsigned hash, n;
	char *keybuf = NULL;
	const char *key;

#undef RETURN
#define RETURN(x) do \
{ \
	if (keybuf) { \
		if (table->slice) \
			acl_slice_pool_free(__FILE__, __LINE__, keybuf); \
		else \
			acl_myfree(keybuf); \
	} \
	return (x); \
} while (0)

	if ((table->flag & ACL_HTABLE_FLAG_KEY_LOWER)) {
		if (table->slice)
			keybuf = acl_slice_pool_strdup(__FILE__, __LINE__,
					table->slice, key_in);
		else
			keybuf = acl_mystrdup(key_in);
		acl_lowercase(keybuf);
		key = keybuf;
	} else
		key = key_in;

	table->status = ACL_HTABLE_STAT_OK;
	hash = table->hash_fn(key, strlen(key));

	if (table->used >= table->size) {
		ret = htable_grow(table);
		if(ret < 0) {
			RETURN (NULL);
		}
	}

	n = hash % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key.c_key)) {
			table->status = ACL_HTABLE_STAT_DUPLEX_KEY;
			acl_msg_info("%s(%d): duplex key(%s) exist",
				myname, __LINE__, key);
			RETURN (ht);
		}
	}

	if (table->slice)
		ht = (ACL_HTABLE_INFO*) acl_slice_pool_alloc(__FILE__, __LINE__,
				table->slice, sizeof(ACL_HTABLE_INFO));
	else
		ht = (ACL_HTABLE_INFO *) acl_mymalloc(sizeof(ACL_HTABLE_INFO));
	if (ht == NULL) {
		acl_msg_error("%s(%d): alloc error", myname, __LINE__);
		RETURN (NULL);
	}

	if ((table->flag & ACL_HTABLE_FLAG_KEY_REUSE))
		ht->key.c_key = key;
	else {
		if (table->slice)
			ht->key.key = acl_slice_pool_strdup(__FILE__, __LINE__,
						table->slice, key);
		else
			ht->key.key = acl_mystrdup(key);
		if (ht->key.key == NULL) {
			acl_msg_error("%s(%d): alloc error", myname, __LINE__);
			if (table->slice)
				acl_slice_pool_free(__FILE__, __LINE__, ht);
			else
				acl_myfree(ht);
			RETURN (NULL);
		}
	}

	ht->hash  = hash;
	ht->value = value;
	htable_link(table, ht, n);
	RETURN (ht);
}

int acl_htable_enter_r(ACL_HTABLE *table, const char *key_in, void *value,
	void (*callback)(ACL_HTABLE_INFO *ht, void *arg), void *arg)
{
	const char *myname = "acl_htable_enter_r";
	ACL_HTABLE_INFO *ht;
	int   ret;
	unsigned hash, n;
	char *keybuf = NULL;
	const char *key;

#undef RETURN
#define RETURN(x) do \
{ \
	if (keybuf) { \
		if (table->slice) \
			acl_slice_pool_free(__FILE__, __LINE__, keybuf); \
		else \
			acl_myfree(keybuf); \
	} \
	return (x); \
} while (0)

	if ((table->flag & ACL_HTABLE_FLAG_KEY_LOWER)) {
		if (table->slice)
			keybuf = acl_slice_pool_strdup(__FILE__, __LINE__,
					table->slice, key_in);
		else
			keybuf = acl_mystrdup(key_in);
		acl_lowercase(keybuf);
		key = keybuf;
	} else
		key = key_in;

	hash = table->hash_fn(key, strlen(key));

	table->status = ACL_HTABLE_STAT_OK;
	LOCK_TABLE_WRITE(table);

	if (table->used >= table->size) {
		ret = htable_grow(table);
		if(ret < 0) {
			UNLOCK_TABLE(table);
			RETURN (-1);
		}
	}

	n = hash % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key.c_key)) {
			acl_msg_info("%s(%d): duplex key(%s) exist",
				myname, __LINE__, key);
			table->status = ACL_HTABLE_STAT_DUPLEX_KEY;
			if (callback)
				callback(ht, arg);
			UNLOCK_TABLE(table);
			RETURN (0);
		}
	}

	if (table->slice)
		ht = (ACL_HTABLE_INFO*) acl_slice_pool_alloc(__FILE__, __LINE__,
				table->slice, sizeof(ACL_HTABLE_INFO));
	else
		ht = (ACL_HTABLE_INFO *) acl_mymalloc(sizeof(ACL_HTABLE_INFO));

	if ((table->flag & ACL_HTABLE_FLAG_KEY_REUSE))
		ht->key.c_key = key;
	else if (table->slice)
		ht->key.key = acl_slice_pool_strdup(__FILE__, __LINE__,
					table->slice, key);
	else
		ht->key.key = acl_mystrdup(key);

	ht->hash  = hash;
	ht->value = value;
	htable_link(table, ht, n);

	if (callback)
		callback(ht, arg);

	UNLOCK_TABLE(table);

	RETURN (0);
}
/* acl_htable_find - lookup value */

void *acl_htable_find(ACL_HTABLE *table, const char *key)
{
	ACL_HTABLE_INFO *ht = acl_htable_locate(table, key);

	return ht != NULL ? ht->value : NULL;
}

int  acl_htable_find_r(ACL_HTABLE *table, const char *key_in,
	void (*callback)(void *value, void *arg), void *arg)
{
	ACL_HTABLE_INFO *ht;
	unsigned  n;
	char *keybuf = NULL;
	const char *key;

#undef RETURN
#define RETURN(x) do \
{ \
	if (keybuf) { \
		if (table->slice) \
			acl_slice_pool_free(__FILE__, __LINE__, keybuf); \
		else \
			acl_myfree(keybuf); \
	} \
	return (x); \
} while (0)

	if ((table->flag & ACL_HTABLE_FLAG_KEY_LOWER)) {
		if (table->slice)
			keybuf = acl_slice_pool_strdup(__FILE__, __LINE__,
					table->slice, key_in);
		else
			keybuf = acl_mystrdup(key_in);
		acl_lowercase(keybuf);
		key = keybuf;
	} else
		key = key_in;

	n = table->hash_fn(key, strlen(key));

	LOCK_TABLE_READ(table);

	n = n % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key.c_key)) {
			if (callback)
				callback(ht->value, arg);

			if (!(table->flag & ACL_HTABLE_FLAG_MSLOOK)) {
				UNLOCK_TABLE(table);
				RETURN (0);
			}
			if (ht == table->data[n]) {
				UNLOCK_TABLE(table);
				RETURN (0);
			}
			if (ht->next) {
				ht->prev->next = ht->next;
				ht->next->prev = ht->prev;
			} else {
				ht->prev->next = NULL;
			}
			table->data[n]->prev = ht;
			ht->prev = NULL;
			ht->next = table->data[n];
			table->data[n] = ht;

			UNLOCK_TABLE(table);
			RETURN (0);
		}
	}

	UNLOCK_TABLE(table);

	RETURN (-1);
}

/* acl_htable_locate - lookup entry */

ACL_HTABLE_INFO *acl_htable_locate(ACL_HTABLE *table, const char *key_in)
{
	ACL_HTABLE_INFO *ht;
	unsigned  n;
	char *keybuf = NULL;
	const char *key;

#undef RETURN
#define RETURN(x) do \
{ \
	if (keybuf) { \
		if (table->slice) \
			acl_slice_pool_free(__FILE__, __LINE__, keybuf); \
		else \
			acl_myfree(keybuf); \
	} \
	return (x); \
} while (0)

	if ((table->flag & ACL_HTABLE_FLAG_KEY_LOWER)) {
		if (table->slice)
			keybuf = acl_slice_pool_strdup(__FILE__, __LINE__,
					table->slice, key_in);
		else
			keybuf = acl_mystrdup(key_in);
		acl_lowercase(keybuf);
		key = keybuf;
	} else
		key = key_in;

	n = table->hash_fn(key, strlen(key));

	n = n % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key.c_key)) {
			if (!(table->flag & ACL_HTABLE_FLAG_MSLOOK))
				RETURN (ht);
			if (ht == table->data[n])
				RETURN (ht);
			if (ht->next) {
				ht->prev->next = ht->next;
				ht->next->prev = ht->prev;
			} else {
				ht->prev->next = NULL;
			}
			table->data[n]->prev = ht;
			ht->prev = NULL;
			ht->next = table->data[n];
			table->data[n] = ht;
			RETURN (ht);
		}
	}

	RETURN (NULL);
}

int acl_htable_locate_r(ACL_HTABLE *table, const char *key_in,
	void (*callback)(ACL_HTABLE_INFO *ht, void *arg), void *arg)
{
	ACL_HTABLE_INFO *ht;
	unsigned  n;
	char *keybuf = NULL;
	const char *key;

#undef RETURN
#define RETURN(x) do \
{ \
	if (keybuf) { \
		if (table->slice) \
			acl_slice_pool_free(__FILE__, __LINE__, keybuf); \
		else \
			acl_myfree(keybuf); \
	} \
	return (x); \
} while (0)

	if ((table->flag & ACL_HTABLE_FLAG_KEY_LOWER)) {
		if (table->slice)
			keybuf = acl_slice_pool_strdup(__FILE__, __LINE__,
					table->slice, key_in);
		else
			keybuf = acl_mystrdup(key_in);
		acl_lowercase(keybuf);
		key = keybuf;
	} else
		key = key_in;

	n = table->hash_fn(key, strlen(key));

	LOCK_TABLE_READ(table);

	n = n % table->size;

	for (ht = table->data[n]; ht; ht = ht->next) {
		if (STREQ(key, ht->key.c_key)) {
			if (callback)
				callback(ht, arg);
			UNLOCK_TABLE(table);
			RETURN (0);
		}
	}

	UNLOCK_TABLE(table);

	RETURN (-1);
}

void acl_htable_delete_entry(ACL_HTABLE *table, ACL_HTABLE_INFO *ht,
	void (*free_fn) (void *))
{
	ACL_HTABLE_INFO **h = table->data + ht->hash % table->size;

	if (ht->next)
		ht->next->prev = ht->prev;
	if (ht->prev)
		ht->prev->next = ht->next;
	else
		*h = ht->next;
	if (!(table->flag & ACL_HTABLE_FLAG_KEY_REUSE)) {
		if (table->slice)
			acl_slice_pool_free(__FILE__, __LINE__, ht->key.key);
		else
			acl_myfree(ht->key.key);
	}
	if (free_fn && ht->value)
		(*free_fn) (ht->value);
	if (table->slice)
		acl_slice_pool_free(__FILE__, __LINE__, ht);
	else
		acl_myfree(ht);
	table->used--;
}

/* acl_htable_delete - delete one entry */

int acl_htable_delete(ACL_HTABLE *table, const char *key_in,
	void (*free_fn) (void *))
{
	ACL_HTABLE_INFO *ht;
	unsigned  n;
	ACL_HTABLE_INFO **h;
	char *keybuf = NULL;
	const char *key;

#undef RETURN
#define RETURN(x) do \
{ \
	if (keybuf) { \
		if (table->slice) \
			acl_slice_pool_free(__FILE__, __LINE__, keybuf); \
		else \
			acl_myfree(keybuf); \
	} \
	return (x); \
} while (0)

	if ((table->flag & ACL_HTABLE_FLAG_KEY_LOWER)) {
		if (table->slice)
			keybuf = acl_slice_pool_strdup(__FILE__, __LINE__,
					table->slice, key_in);
		else
			keybuf = acl_mystrdup(key_in);
		acl_lowercase(keybuf);
		key = keybuf;
	} else
		key = key_in;

	n = table->hash_fn(key, strlen(key));

	LOCK_TABLE_WRITE(table);

	n = n % table->size;

	h = table->data + n;
	for (ht = *h; ht; ht = ht->next) {
		if (STREQ(key, ht->key.c_key)) {
			acl_htable_delete_entry(table, ht, free_fn);
			UNLOCK_TABLE(table);
			RETURN(0);
		}
	}

	UNLOCK_TABLE(table);
	RETURN(-1);
}

/* acl_htable_free - destroy hash table */

void acl_htable_free(ACL_HTABLE *table, void (*free_fn) (void *))
{
	unsigned i = table->size;
	ACL_HTABLE_INFO *ht;
	ACL_HTABLE_INFO *next;
	ACL_HTABLE_INFO **h = table->data;

	while (i-- > 0) {
		for (ht = *h++; ht; ht = next) {
			next = ht->next;
			if (!(table->flag & ACL_HTABLE_FLAG_KEY_REUSE)) {
				if (table->slice)
					acl_slice_pool_free(__FILE__, __LINE__,
						ht->key.key);
				else
					acl_myfree(ht->key.key);
			}
			if (free_fn && ht->value)
				(*free_fn) (ht->value);
			if (table->slice)
				acl_slice_pool_free(__FILE__, __LINE__, ht);
			else
				acl_myfree(ht);
		}
	}

	if (table->slice)
		acl_slice_pool_free(__FILE__, __LINE__, table->data);
	else
		acl_myfree(table->data);
	table->data = 0;
	if (table->rwlock) {
		_RWLOCK_DESTROY(table->rwlock);
		if (table->slice)
			acl_slice_pool_free(__FILE__, __LINE__, table->rwlock);
		else
			acl_myfree(table->rwlock);
	}

	if (table->slice)
		acl_slice_pool_free(__FILE__, __LINE__, table);
	else
		acl_myfree(table);
}

int acl_htable_reset(ACL_HTABLE *table, void (*free_fn) (void *))
{
	unsigned i = table->size;
	ACL_HTABLE_INFO *ht;
	ACL_HTABLE_INFO *next;
	ACL_HTABLE_INFO **h;
	int ret;

	LOCK_TABLE_WRITE(table);

	h = table->data;

	while (i-- > 0) {
		for (ht = *h++; ht; ht = next) {
			next = ht->next;
			if (!(table->flag & ACL_HTABLE_FLAG_KEY_REUSE)) {
				if (table->slice)
					acl_slice_pool_free(__FILE__, __LINE__, ht->key.key);
				else
					acl_myfree(ht->key.key);
			}
			if (free_fn && ht->value)
				(*free_fn) (ht->value);
			if (table->slice)
				acl_slice_pool_free(__FILE__, __LINE__, ht);
			else
				acl_myfree(ht);
		}
	}
	if (table->slice)
		acl_slice_pool_free(__FILE__, __LINE__, table->data);
	else
		acl_myfree(table->data);
	ret = htable_size(table, table->init_size < 13 ? 13 : table->init_size);

	UNLOCK_TABLE(table);
	return (ret);
}

const ACL_HTABLE_INFO *acl_htable_iter_head(ACL_HTABLE *table, ACL_HTABLE_ITER *iter)
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

const ACL_HTABLE_INFO *acl_htable_iter_next(ACL_HTABLE_ITER *iter)
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

const ACL_HTABLE_INFO *acl_htable_iter_tail(ACL_HTABLE *table, ACL_HTABLE_ITER *iter)
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

const ACL_HTABLE_INFO *acl_htable_iter_prev(ACL_HTABLE_ITER *iter)
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

/* acl_htable_walk - iterate over hash table */

void acl_htable_walk(ACL_HTABLE *table, void (*action)(ACL_HTABLE_INFO *, void *), void *arg)
{
	unsigned i = table->size;
	ACL_HTABLE_INFO **h = table->data;
	ACL_HTABLE_INFO *ht;

	LOCK_TABLE_READ(table);
	while (i-- > 0)
		for (ht = *h++; ht; ht = ht->next)
			(*action) (ht, arg);
	UNLOCK_TABLE(table);
}

int acl_htable_size(const ACL_HTABLE *table)
{
	if (table)
		return (table->size);
	else
		return (0);
}

int acl_htable_used(const ACL_HTABLE *table)
{
	if (table)
		return (table->used);
	else
		return (0);
}

ACL_HTABLE_INFO **acl_htable_data(ACL_HTABLE *table)
{
	return ((ACL_HTABLE_INFO**) table->data);
}

/* acl_htable_list - list all table members */

ACL_HTABLE_INFO **acl_htable_list(const ACL_HTABLE *table)
{
	ACL_HTABLE_INFO **list;
	ACL_HTABLE_INFO *member;
	int     count = 0;
	int     i;

	if (table != 0) {
		list = (ACL_HTABLE_INFO **) acl_mymalloc(sizeof(*list) * (table->used + 1));
		for (i = 0; i < table->size; i++)
			for (member = table->data[i]; member != 0; member = member->next)
				list[count++] = member;
	} else {
		list = (ACL_HTABLE_INFO **) acl_mymalloc(sizeof(*list));
	}
	list[count] = 0;
	return (list);
}

void acl_htable_stat(const ACL_HTABLE *table)
{
	ACL_HTABLE_INFO *member;
	int	i, count;

	LOCK_TABLE_READ(table);
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
	UNLOCK_TABLE(table);
}
