#include "stdafx.h"
#include "service_main.h"

static ACL_AIO *__aio = NULL;
static ACL_HTABLE *__cache = NULL;

void ns_cache_init(ACL_AIO *aio)
{
	const char *myname = "ns_cache_init";

	if (__cache != NULL)
		acl_msg_fatal("%s(%d): __cache != NULL", myname, __LINE__);

	__aio = aio;
	__cache = acl_htable_create(10000, 0);
}

static void ns_cache_timeout_fn(int event_type acl_unused,
	ACL_EVENT *event acl_unused, void *context)
{
	NS_CACHE_ENTRY *entry = (NS_CACHE_ENTRY*) context;

	acl_htable_delete(__cache, entry->domain, NULL);
	ns_cache_entry_free(entry);
}

void ns_cache_add(NS_CACHE_ENTRY *entry)
{
	const char *myname = "ns_cache_add";
	NS_CACHE_ENTRY *old_entry;

	if (__cache == NULL)
		acl_msg_fatal("%s(%d): ns_cache_init not called",
			myname, __LINE__);

	old_entry = (NS_CACHE_ENTRY*) acl_htable_find(__cache, entry->domain);
	if (old_entry == NULL) {
		acl_htable_enter(__cache, entry->domain, (char*) entry);
		if (entry->ttl > 0)
			acl_aio_request_timer(__aio, ns_cache_timeout_fn,
				entry, entry->ttl, 0);
		return;
	}

	if (entry->ttl > 0) {
		acl_aio_cancel_timer(__aio, ns_cache_timeout_fn, old_entry);
		acl_aio_request_timer(__aio, ns_cache_timeout_fn,
			entry, entry->ttl, 0);
	}
	if (entry == old_entry)
		return;
	acl_htable_delete(__cache, old_entry->domain, NULL);
	ns_cache_entry_free(old_entry);
	acl_htable_enter(__cache, entry->domain, (char*) entry);
}

NS_CACHE_ENTRY *ns_cache_find(const char *domain)
{
	const char *myname = "ns_cache_find";
	NS_CACHE_ENTRY *entry;
	char  key[MAX_DOMAIN_LEN];

	if (__cache == NULL)
		acl_msg_fatal("%s(%d): ns_cache_init not called",
			myname, __LINE__);

	ACL_SAFE_STRNCPY(key, domain, sizeof(key));
	acl_lowercase(key);

	entry = (NS_CACHE_ENTRY*) acl_htable_find(__cache, key);
	return (entry);
}

int ns_cache_delete(const char *domain)
{
	const char *myname = "ns_cache_delete";
	char  key[MAX_DOMAIN_LEN];

	if (__cache == NULL)
		acl_msg_fatal("%s(%d): ns_cache_init not called",
			myname, __LINE__);

	ACL_SAFE_STRNCPY(key, domain, sizeof(key));
	acl_lowercase(key);

	return (acl_htable_delete(__cache, key,
			(void (*)(void*))ns_cache_entry_free));
}

void ns_cache_entry_free(NS_CACHE_ENTRY *entry)
{
	if (entry->ip_list)
		acl_argv_free(entry->ip_list);
	acl_myfree(entry);
}

NS_CACHE_ENTRY *ns_cache_entry_new(const char *domain, const ACL_ARGV *ip_list,
	const char *cache_buf, int dlen, int ttl)
{
	const char *myname = "ns_cache_entry_new";
	NS_CACHE_ENTRY *entry;
	int   i;

	if (ip_list == NULL && (cache_buf == NULL || dlen <= 0)) {
		acl_msg_error("%s(%d): invalid input args", myname, __LINE__);
		return (NULL);
	}
	if (ip_list != NULL && (cache_buf != NULL && dlen > 0)) {
		acl_msg_error("%s(%d): ip_list != NULL and cache_buf != NULL",
			myname, __LINE__);
		return (NULL);
	}

	entry = (NS_CACHE_ENTRY*) acl_mycalloc(1, sizeof(NS_CACHE_ENTRY));
	entry->idx = 0;
	entry->ttl = ttl;
	ACL_SAFE_STRNCPY(entry->domain, domain, sizeof(entry->domain));
	acl_lowercase(entry->domain);

	if (ip_list) {
		entry->ip_list = acl_argv_alloc(10);
		for (i = 0; i < ip_list->argc; i++) {
			acl_argv_add(entry->ip_list, ip_list->argv[i], NULL);
		}
		entry->cache_dlen = 0;
	} else if (cache_buf) {
		memcpy(entry->cache_buf, cache_buf, dlen);
		entry->cache_dlen = dlen;
	}

	return (entry);
}

static char __unknown_domain[MAX_DOMAIN_LEN];
static NS_CACHE_ENTRY *__unknown_cache_entry;

void ns_cache_add_unknown(const char *domain, const ACL_ARGV *ip_list)
{
	__unknown_cache_entry = ns_cache_entry_new(domain, ip_list, NULL, 0, 0);
	ACL_SAFE_STRNCPY(__unknown_domain, domain, sizeof(__unknown_domain));
}

NS_CACHE_ENTRY *ns_cache_unknown(void)
{
	const char *myname = "ns_cache_unknown";

	if (__unknown_cache_entry == NULL)
		acl_msg_fatal("%s(%d): __unknown_cache_entry null",
			myname, __LINE__);
	return (__unknown_cache_entry);
}

ACL_ARGV *ns_cache_ip_list(NS_CACHE_ENTRY *cache_entry)
{
	ACL_ARGV *argv = acl_argv_alloc(10);
	ACL_ARGV *ip_list = cache_entry->ip_list;
	int   i, n;

	n = ip_list->argc;
	if (cache_entry->idx >= n)
		cache_entry->idx = 0;
	i = cache_entry->idx++;
	while (n-- > 0) {
		acl_argv_add(argv, ip_list->argv[i++], NULL);
		if (i >= ip_list->argc)
			i = 0;
	}

	return (argv);
}
