#include "lib_acl.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "dns.h"

static int __thread_safe;

typedef struct CACHE {
	ACL_DNS_DB *dns_db;
	char  name[256];
	time_t tm_timeout;
	ACL_RING ring_entry;
} CACHE;

struct DNS_CACHE {
	acl_pthread_mutex_t cache_mutex;
	ACL_HTABLE *cache_table;
	ACL_RING  cache_ring;
};

static int  __use_trylock = 0;
static int  __cache_timeout = 300;  /* 300 seconds */

static void cache_lock(DNS_CACHE *dns_cache)
{
	const char *myname = "cache_lock";
	char  buf[256];
	int   status;

	if (__thread_safe == 0)
		return;

	status = acl_pthread_mutex_lock(&dns_cache->cache_mutex);
	if (status) {
		acl_msg_fatal("%s: pthread_mutex_lock error(%s)",
			myname, acl_last_strerror(buf, sizeof(buf)));
	}
}

static int cache_trylock(DNS_CACHE *dns_cache)
{
	int   status;

	if (__thread_safe == 0)
		return (0);

	status = acl_pthread_mutex_trylock(&dns_cache->cache_mutex);
	if (status == 0)
		return (0);
	else {
		return (-1);
	}
}

static void cache_unlock(DNS_CACHE *dns_cache)
{
	const char *myname = "cache_unlock";
	char  buf[256];
	int   status;

	if (__thread_safe == 0)
		return;

	status = acl_pthread_mutex_unlock(&dns_cache->cache_mutex);
	if (status) {
		acl_msg_fatal("%s: pthread_mutex_lock error(%s)",
			myname, acl_last_strerror(buf, sizeof(buf)));
	}
}

static void free_cache_fn(DNS_CACHE *dns_cache, CACHE *cache)
{
	if (cache->dns_db)
		acl_netdb_free(cache->dns_db);
	acl_ring_detach(&cache->ring_entry);
	acl_htable_delete(dns_cache->cache_table, cache->name, NULL);
	acl_myfree(cache);
}

static void cache_timer_fn(DNS_CACHE *dns_cache)
{
	CACHE *cache, *tmp;
	ACL_RING *iter;
	time_t current;

	current = time(NULL);

	for (iter = acl_ring_succ(&dns_cache->cache_ring);
		iter != &dns_cache->cache_ring;)
	{
		cache = acl_ring_to_appl(iter, CACHE, ring_entry);
		if (current >= cache->tm_timeout) {
			tmp = cache;
			iter = acl_ring_succ(iter);
			free_cache_fn(dns_cache, tmp);
		} else
			break;
	}
}

void dns_cache_push_one(DNS_CACHE *dns_cache, const ACL_DNS_DB *dns_db, int timeout)
{
	const char *myname = "dns_cache_push_one";
	CACHE *cache;

	if (dns_db == NULL || dns_db->h_db == NULL) {
		acl_msg_error("%s(%d): input invalid", myname, __LINE__);
		return;
	}
	if (dns_db->name[0] == 0) {
		acl_msg_error("%s(%d): host name empty", myname, __LINE__);
		return;
	}
	if (dns_db->size <= 0) {
		acl_msg_error("%s(%d): size(%d) <= 0", myname, __LINE__, dns_db->size);
		return;
	}
	if (dns_db->size != acl_array_size(dns_db->h_db)) {
		acl_msg_fatal("%s(%d): size(%d) != array size(%d)",
			myname, __LINE__, dns_db->size,
			acl_array_size(dns_db->h_db));
	}

	if (__use_trylock) {
		if (cache_trylock(dns_cache) < 0) {
			return;
		}
	} else
		cache_lock(dns_cache);

	cache = (CACHE *) acl_htable_find(dns_cache->cache_table, dns_db->name);
	if (cache == NULL) {
		cache = (CACHE *) acl_mycalloc(1, sizeof(CACHE));
		if (cache == NULL) {
			cache_unlock(dns_cache);
			acl_msg_error("%s(%d): calloc error(%s)",
				myname, __LINE__, acl_last_serror());
			return;
		}

		if (acl_htable_enter(dns_cache->cache_table,
			dns_db->name, (char *) cache) == NULL)
		{
			cache_unlock(dns_cache);
			acl_myfree(cache);
			acl_msg_error("%s(%d): add to htable error(%s)",
				myname, __LINE__, acl_last_serror());
			return;
		}

		acl_ring_prepend(&dns_cache->cache_ring, &cache->ring_entry);
		ACL_SAFE_STRNCPY(cache->name, dns_db->name, sizeof(cache->name));
	} else {
		/* override the old cache */
		if (cache->dns_db != NULL)
			acl_netdb_free(cache->dns_db);
		cache->dns_db = NULL;
		acl_ring_detach(&cache->ring_entry);
		acl_ring_prepend(&dns_cache->cache_ring, &cache->ring_entry);
	}

	cache->tm_timeout = time(NULL) + timeout;

	cache->dns_db = acl_netdb_clone(dns_db);
	if (cache->dns_db == NULL) {
		free_cache_fn(dns_cache, cache);
	}

	cache_unlock(dns_cache);
}

void dns_cache_push(DNS_CACHE *dns_cache, const ACL_DNS_DB *dns_db)
{
	dns_cache_push_one(dns_cache, dns_db, __cache_timeout);
}

void dns_cache_push2(DNS_CACHE *dns_cache, const DNS_CTX *dns_ctx)
{
	const char *myname = "dns_cache_push2";
	ACL_DNS_DB *dns_db;
	CACHE *cache;
	int i;

	if (dns_ctx == NULL) {
		acl_msg_error("%s(%d): input invalid", myname, __LINE__);
		return;
	}
	if (dns_ctx->domain_key[0] == 0) {
		acl_msg_error("%s(%d): domain_key empty", myname, __LINE__);
		return;
	}
	if (dns_ctx->ip_cnt <= 0) {
		acl_msg_error("%s(%d): size(%d) <= 0",
			myname, __LINE__, dns_ctx->ip_cnt);
		return;
	}

	dns_db = acl_netdb_new(dns_ctx->domain_key);

	for (i = 0; i < dns_ctx->ip_cnt; i++) {
		acl_netdb_addip(dns_db, dns_ctx->ip[i]);
	}
	
	if (__use_trylock) {
		if (cache_trylock(dns_cache) < 0) {
			acl_netdb_free(dns_db);
			return;
		}
	} else
		cache_lock(dns_cache);

	cache = (CACHE *) acl_htable_find(dns_cache->cache_table, dns_ctx->domain_key);
	if (cache == NULL) {
		cache = (CACHE *) acl_mycalloc(1, sizeof(CACHE));
		if (cache == NULL) {
			cache_unlock(dns_cache);
			acl_netdb_free(dns_db);
			acl_msg_error("%s(%d): calloc error(%s)",
				myname, __LINE__, acl_last_serror());
			return;
		}

		if (acl_htable_enter(dns_cache->cache_table,
			dns_db->name, (char *) cache) == NULL)
		{
			cache_unlock(dns_cache);
			acl_netdb_free(dns_db);
			acl_msg_error("%s(%d): add to htable error(%s)",
				myname, __LINE__, acl_last_serror());
			return;
		}

		acl_ring_append(&dns_cache->cache_ring, &cache->ring_entry);

		ACL_SAFE_STRNCPY(cache->name, dns_db->name, sizeof(cache->name));
	} else {
		/* override the old cache */

		if (cache->dns_db != NULL)
			acl_netdb_free(cache->dns_db);
		cache->dns_db = NULL;
		acl_ring_detach(&cache->ring_entry);
		acl_ring_append(&dns_cache->cache_ring, &cache->ring_entry);
	}

	cache->tm_timeout = time(NULL) + __cache_timeout;
	cache->dns_db = dns_db;

	cache_unlock(dns_cache);
}

ACL_DNS_DB *dns_cache_lookup(DNS_CACHE *dns_cache, const char *name)
{
	const char *myname = "dns_cache_lookup";
	CACHE *cache;
	char  buf[256];
	ACL_DNS_DB *dns_db;

	if (__use_trylock) {
		if (cache_trylock(dns_cache) < 0)
			return (NULL);
	} else
		cache_lock(dns_cache);

	cache_timer_fn(dns_cache); /* 先启动定时清理器，将过期的DNS解析去掉 */

	ACL_SAFE_STRNCPY(buf, name, sizeof(buf));
	acl_lowercase(buf);

	cache = (CACHE *) acl_htable_find(dns_cache->cache_table, buf);
	if (cache == NULL) {
		cache_unlock(dns_cache);
		return (NULL);
	}

	if (cache->dns_db == NULL) {	/* XXX */
		acl_msg_error("%s, %s(%d): dns_db null",
			__FILE__, myname, __LINE__);
		free_cache_fn(dns_cache, cache);
		cache_unlock(dns_cache);
		return (NULL);
	}

	/* if the dns cache has been timeout ? */

	if (time(NULL) >= cache->tm_timeout) {
		free_cache_fn(dns_cache, cache);
		cache_unlock(dns_cache);
		return (NULL);
	}

	/* clone the ACL_DNS_DB object */

	dns_db = acl_netdb_clone(cache->dns_db);
	if (dns_db == NULL) {
		acl_msg_error("%s, %s(%d): calloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
		cache_unlock(dns_cache);
		return (NULL);
	}

	cache_unlock(dns_cache);

	return (dns_db);
}

void dns_cache_del_host(DNS_CACHE *dns_cache, const char *name)
{
	CACHE *cache;

	cache_lock(dns_cache);
	cache = (CACHE *) acl_htable_find(dns_cache->cache_table, name);
	if (cache)
		free_cache_fn(dns_cache, cache);
	cache_unlock(dns_cache);
}

DNS_CACHE *dns_cache_create(int timeout, int thread_safe)
{
	const char *myname = "dns_cache_create";
	int   status;
	DNS_CACHE *dns_cache = (DNS_CACHE*) acl_mycalloc(1, sizeof(DNS_CACHE));

	if (timeout > 0)
		__cache_timeout = timeout;

	if (thread_safe) {
		status = acl_pthread_mutex_init(&dns_cache->cache_mutex, NULL);
		if (status) {
			acl_msg_fatal("%s: pthread_mutex_init error(%s)",
				myname, acl_last_serror());
		}
		__use_trylock = 1;
		__thread_safe = 1;
	} else
		__thread_safe = 0;

	dns_cache->cache_table = acl_htable_create(256, 0);
	if (dns_cache->cache_table == NULL)
		acl_msg_fatal("%s: create htable error(%s)",
			myname, acl_last_serror());

	acl_ring_init(&dns_cache->cache_ring);
	return (dns_cache);
}
