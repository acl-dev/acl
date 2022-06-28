#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdio.h>
#ifdef  ACL_WINDOWS
#include <io.h>
#endif
#ifdef  ACL_UNIX
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_stdlib.h"
#include "thread/acl_thread.h"
#include "net/acl_netdb.h"

#endif

static acl_pthread_mutex_t __cache_mutex;
static ACL_HTABLE *__cache_table = NULL;
static ACL_RING  __cache_ring;

typedef struct CACHE {
	ACL_DNS_DB *h_dns_db;
	char  name[256];
	time_t tm_timeout;
	ACL_RING ring_entry;
} CACHE;

static int  __use_trylock = 0;
static int  __cache_timeout = 300;  /* 300 seconds */

static void cache_lock(void)
{
	const char *myname = "cache_lock";
	char  buf[256];
	int   status;

	status = acl_pthread_mutex_lock(&__cache_mutex);
	if (status) {
		acl_msg_fatal("%s: pthread_mutex_lock error(%s)",
				myname, acl_last_strerror(buf, sizeof(buf)));
	}
}

static int cache_trylock(void)
{
	int   status;

	status = acl_pthread_mutex_trylock(&__cache_mutex);
	if (status == 0)
		return (0);
	else {
		return (-1);
	}
}

static void cache_unlock(void)
{
	const char *myname = "cache_unlock";
	char  buf[256];
	int   status;

	status = acl_pthread_mutex_unlock(&__cache_mutex);
	if (status) {
		acl_msg_fatal("%s: pthread_mutex_lock error(%s)",
				myname, acl_last_strerror(buf, sizeof(buf)));
	}
}

static void free_cache_fn(char *value)
{
	CACHE *cache = (CACHE *) value;

	if (cache->h_dns_db)
		acl_netdb_free(cache->h_dns_db);
	acl_ring_detach(&cache->ring_entry);
	acl_htable_delete(__cache_table, cache->name, NULL);
	acl_myfree(cache);
}

static void cache_timer_fn(void)
{
	CACHE *cache, *tmp;
	ACL_RING_ITER iter;
	time_t current;

	current = time(NULL);

	acl_ring_foreach(iter, &__cache_ring) {
		cache = ACL_RING_TO_APPL(iter.ptr, CACHE, ring_entry);
		if (current >= cache->tm_timeout) {
			tmp = cache;
			iter.ptr = acl_ring_succ(iter.ptr);
			free_cache_fn((char *) tmp);
		} else
			break;
	}
}

void acl_netdb_cache_push(const ACL_DNS_DB *h_dns_db, int timeout)
{
	const char *myname = "acl_netdb_cache_push";
	char  buf[256];
	CACHE *cache;

	/* 如果禁止缓存，则直接返回 */
	if (__cache_timeout <= 0)
		return;

	if (__cache_table == NULL)
		return;

	if (h_dns_db == NULL || h_dns_db->h_db == NULL) {
		acl_msg_error("%s(%d): input invalid", myname, __LINE__);
		return;
	}
	if (__cache_table == NULL) {
		acl_msg_error("%s(%d): acl_netdb_cache_init first", myname, __LINE__);
	}
	if (h_dns_db->name[0] == 0) {
		acl_msg_error("%s(%d): host name empty", myname, __LINE__);
		return;
	}
	if (h_dns_db->size <= 0) {
		acl_msg_error("%s(%d): size(%d) <= 0", myname, __LINE__, h_dns_db->size);
		return;
	}
	if (h_dns_db->size != acl_array_size(h_dns_db->h_db)) {
		acl_msg_fatal("%s(%d): size(%d) != array size(%d)",
				myname, __LINE__, h_dns_db->size,
				acl_array_size(h_dns_db->h_db));
	}

	if (__use_trylock) {
		if (cache_trylock() < 0) {
			return;
		}
	} else
		cache_lock();

	cache = (CACHE *) acl_htable_find(__cache_table, h_dns_db->name);
	if (cache == NULL) {
		cache = (CACHE *) acl_mycalloc(1, sizeof(CACHE));
		if (cache == NULL) {
			cache_unlock();
			acl_msg_error("%s(%d): calloc error(%s)",
				myname, __LINE__, acl_last_strerror(buf, sizeof(buf)));
			return;
		}

		if (acl_htable_enter(__cache_table, h_dns_db->name, (char *) cache) == NULL) {
			cache_unlock();
			acl_msg_error("%s(%d): add to htable error(%s)",
				myname, __LINE__, acl_last_strerror(buf, sizeof(buf)));
			return;
		}

		acl_ring_prepend(&__cache_ring, &cache->ring_entry);

		ACL_SAFE_STRNCPY(cache->name, h_dns_db->name, sizeof(cache->name));
	} else {
		/* override the old cache */

		if (cache->h_dns_db != NULL)
			acl_netdb_free(cache->h_dns_db);
		cache->h_dns_db = NULL;
		acl_ring_detach(&cache->ring_entry);
		acl_ring_prepend(&__cache_ring, &cache->ring_entry);
	}

	cache->tm_timeout = time(NULL) + timeout > 0 ? timeout : __cache_timeout;
	cache->h_dns_db = acl_netdb_clone(h_dns_db);
	if (cache->h_dns_db == NULL) {
		free_cache_fn((char *) cache);
	}

	cache_unlock();
}

ACL_DNS_DB *acl_netdb_cache_lookup(const char *name)
{
	const char *myname = "acl_netdb_cache_lookup";
	CACHE *cache;
	char  buf[256];
	ACL_DNS_DB *h_dns_db = NULL;

	if (__cache_table == NULL || name == NULL)
		return (NULL);

	if (__use_trylock) {
		if (cache_trylock() < 0)
			return (NULL);
	} else
		cache_lock();

	cache_timer_fn();

	ACL_SAFE_STRNCPY(buf, name, sizeof(buf));
	acl_lowercase(buf);

	cache = (CACHE *) acl_htable_find(__cache_table, buf);
	if (cache == NULL) {
		cache_unlock();
		return (NULL);
	}

	if (cache->h_dns_db == NULL) {	/* XXX */
		acl_msg_error("%s, %s(%d): h_dns_db null",
				__FILE__, myname, __LINE__);
		free_cache_fn((char *) cache);
		cache_unlock();
		return (NULL);
	}

	/* if the dns cache has been timeout ? */

	if (time(NULL) >= cache->tm_timeout) {
		free_cache_fn((char *) cache);
		cache_unlock();
		return (NULL);
	}

	/* clone the ACL_DNS_DB object */

	h_dns_db = acl_netdb_clone(cache->h_dns_db);
	if (h_dns_db == NULL) {
		acl_msg_error("%s, %s(%d): calloc error(%s)",
				__FILE__, myname, __LINE__,
				acl_last_strerror(buf, sizeof(buf)));
		cache_unlock();
		return (NULL);
	}

	cache_unlock();

	return (h_dns_db);
}

void acl_netdb_cache_del_host(const char *name)
{
	CACHE *cache;

	if (__cache_table == NULL)
		return;

	cache_lock();
	cache = (CACHE *) acl_htable_find(__cache_table, name);
	if (cache)
		free_cache_fn((char *) cache);
	cache_unlock();
}

void acl_netdb_cache_init(int timeout, int thread_safe)
{
	const char *myname = "acl_netdb_cache_init";
	char  buf[256];
	int   status;

	if (timeout > 0)
		__cache_timeout = timeout;

	if (thread_safe) {
		status = acl_pthread_mutex_init(&__cache_mutex, NULL);
		if (status) {
			acl_msg_error("%s: pthread_mutex_init error(%s)",
				myname, acl_last_strerror(buf, sizeof(buf)));
			return;
		}
		__use_trylock = 1;
	}

	__cache_table = acl_htable_create(256, 0);
	if (__cache_table == NULL)
		acl_msg_error("%s: create htable error(%s)",
				myname, acl_last_strerror(buf, sizeof(buf)));

	acl_ring_init(&__cache_ring);
}


