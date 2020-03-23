#include "StdAfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http.h"
#include "http/lib_http.h"

static void __hdr_init(HTTP_HDR_RES *hh)
{
	hh->reply_status = 0;
}

static void thread_cache_free(ACL_ARRAY *pool)
{
	if ((unsigned long) acl_pthread_self() != acl_main_thread_self())
		acl_array_free(pool, (void (*)(void*)) http_hdr_res_free);
}

static acl_pthread_key_t cache_key = (acl_pthread_key_t) -1;

#ifndef	USE_TLS_EX
static acl_pthread_once_t once_control = ACL_PTHREAD_ONCE_INIT;

static ACL_ARRAY *main_cache = NULL;
# ifndef HAVE_NO_ATEXIT
static void main_cache_free(void)
{
	if (main_cache) {
		acl_array_free(main_cache, (void (*)(void*)) http_hdr_res_free);
		/* 需要将该全局值设为 NULL，以便于 http_hdr_res_free 中的判断 */
		main_cache = NULL;
	}
}
# endif

static void thread_cache_init(void)
{
	acl_pthread_key_create(&cache_key, (void (*)(void*)) thread_cache_free);
}
#endif

/* 生成一个新的 HTTP_HDR_RES 数据结构 */

HTTP_HDR_RES *http_hdr_res_new(void)
{
	HTTP_HDR_RES *hh;
	ACL_ARRAY *pool;

	if (var_http_tls_cache <= 0) {
		hh = (HTTP_HDR_RES *) http_hdr_new(sizeof(HTTP_HDR_RES));
		__hdr_init(hh);
		return (hh);
	}

#ifdef	USE_TLS_EX
	pool = (ACL_ARRAY*) acl_pthread_tls_get(&cache_key);
	if (pool == NULL) {
		pool = acl_array_create(100);
		acl_pthread_tls_set(cache_key, pool,
			(void (*)(void*)) thread_cache_free);
	}

	pool = (ACL_ARRAY*) acl_pthread_tls_get(&cache_key);
	hh = (HTTP_HDR_RES*) pool->pop_back(pool);
	if (hh) {
		http_hdr_reset((HTTP_HDR *) hh);
		__hdr_init(hh);
		return (hh);
	}
#else
	acl_pthread_once(&once_control, thread_cache_init);
	pool = (ACL_ARRAY*) acl_pthread_getspecific(cache_key);
	if (pool == NULL) {
		pool = acl_array_create(100);
		acl_pthread_setspecific(cache_key, pool);
#ifndef HAVE_NO_ATEXIT
		if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
			main_cache = pool;
			atexit(main_cache_free);
		}
#endif
	}
	hh = (HTTP_HDR_RES*) pool->pop_back(pool);
	if (hh) {
		http_hdr_reset((HTTP_HDR *) hh);
		__hdr_init(hh);
		return (hh);
	}
#endif

	hh = (HTTP_HDR_RES *) http_hdr_new(sizeof(HTTP_HDR_RES));
	__hdr_init(hh);
	return (hh);
}

HTTP_HDR_RES *http_hdr_res_clone(const HTTP_HDR_RES *hdr_res)
{
	HTTP_HDR_RES *hh;

	hh = http_hdr_res_new();
	http_hdr_clone(&hdr_res->hdr, &hh->hdr);

	hh->reply_status = hdr_res->reply_status;
	return (hh);
}

static void __hdr_free_member(HTTP_HDR_RES *hh acl_unused)
{
}

/* 释放一个 HTTP_HDR_RES 结构 */

void http_hdr_res_free(HTTP_HDR_RES *hh)
{
	ACL_ARRAY *pool;

	if (hh == NULL)
		return;

	/**
	 * bugfix: 增加判断 main_cache 是否为 NULL，在 lib_acl_cpp 中采用
	 * 单例模式下，单例的释放会比 main_cache_free 调用过程更晚，所以
	 * 会造成内存冲突 --- zsx, 2015.11.6
	 */
	if (var_http_tls_cache <= 0 || main_cache == NULL) {
		__hdr_free_member(hh);
		http_hdr_free((HTTP_HDR *) hh);
		return;
	}

#ifdef	USE_TLS_EX
	pool = (ACL_ARRAY*) acl_pthread_tls_get(&cache_key);
	if (pool != NULL) {
		pool->push_back(pool, hh);
		return;
	}
#else
	pool = (ACL_ARRAY*) acl_pthread_getspecific(cache_key);
	if (pool != NULL && acl_array_size(pool) < var_http_tls_cache) {
		pool->push_back(pool, hh);
		return;
	}
#endif
	__hdr_free_member(hh);
	http_hdr_free((HTTP_HDR *) hh);
}

void http_hdr_res_reset(HTTP_HDR_RES *hh)
{
	if (hh == NULL)
		return;

	http_hdr_reset((HTTP_HDR *) hh);
	__hdr_free_member(hh);
	__hdr_init(hh);
}

int http_hdr_res_status_parse(HTTP_HDR_RES *hh, const char *dbuf)
{
	char  myname[] = "http_hdr_res_status_parse";
	ACL_ARGV *status_argv;
	char *ptr;

	if (hh == NULL || dbuf == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
				__FILE__, myname, __LINE__);
		return (-1);
	}

	/* data: HTTP/1.1 XXX info */

	status_argv = acl_argv_split(dbuf, "\t ");
	if (status_argv->argc < 2) {
		acl_msg_error("%s, %s(%d): invalid reply status line=%s",
				__FILE__, myname, __LINE__, dbuf);
		acl_argv_free(status_argv);
		return (-1);
	}

	ptr = acl_argv_index(status_argv, 0);
	if (http_hdr_parse_version(&hh->hdr, ptr) < 0) {
		acl_argv_free(status_argv);
		return (-1);
	}

	ptr = acl_argv_index(status_argv, 1);
	if (ptr == NULL)
		acl_msg_fatal("%s, %s(%d): null value, idx=1, argc=%d",
				__FILE__, myname, __LINE__, status_argv->argc);
	hh->reply_status = atoi(ptr);
	if (hh->reply_status < 0) {
		acl_msg_error("%s, %s(%d): invalid status(%d)",
				__FILE__, myname, __LINE__, hh->reply_status);
		acl_argv_free(status_argv);
		return (-1);
	}
	acl_argv_free(status_argv);
	return (0);
}

int http_hdr_res_parse(HTTP_HDR_RES *hdr_res)
{
	const char *myname = "http_hdr_res_parse";
	HTTP_HDR *hdr = (HTTP_HDR *) hdr_res;
	HTTP_HDR_ENTRY *entry;
	char *ptr;
	char  buf[32]; /* 2xx, 3xx, 4xx, 5xx */
	int   n;

	if (hdr == NULL)
		acl_msg_fatal("%s: hdr_res null", myname);
	if (hdr->entry_lnk == NULL)
		acl_msg_fatal("%s: entry_lnk null", myname);
	n = acl_array_size(hdr->entry_lnk);
	if (n <= 0) {
		acl_msg_error("%s: entry_lnk's size %d invalid", myname, n);
		return (-1);
	}

	/* data format: xxx info */
	entry = (HTTP_HDR_ENTRY *) acl_array_index(hdr->entry_lnk, 0);

	ptr = entry->value;
	while (*ptr == ' ' || *ptr == '\t')
		ptr++;
	if (*ptr == 0) {
		acl_msg_error("%s: status empty", myname);
		return (-1);
	}

	snprintf(buf, sizeof(buf), "%s", ptr);
	ptr = buf;
	while (*ptr) {
		if (*ptr == ' ' || *ptr == '\t') {
			*ptr = 0;
			break;
		}
		ptr++;
	}

	hdr_res->reply_status = atoi(buf);
#if 0
	if (hdr_res->reply_status < 100 || hdr_res->reply_status >= 600) {
		acl_msg_error("%s: status(%s) invalid", myname, buf);
		return (-1);
	}
#endif

	return (http_hdr_parse(hdr));
}

int http_hdr_res_range(const HTTP_HDR_RES *hdr_res, http_off_t *range_from,
	http_off_t *range_to, http_off_t *total_length)
{
	const char* myname = "http_hdr_res_range";
	const char* value;
	char  buf[256], *ptr, *pfrom, *pto;

	if (hdr_res == NULL)
		acl_msg_fatal("%s(%d): hdr_res null", myname, __LINE__);
	if (range_from == NULL)
		acl_msg_fatal("%s(%d): range_from null", myname, __LINE__);
	if (range_to == NULL)
		acl_msg_fatal("%s(%d): range_to null", myname, __LINE__);

	value = http_hdr_entry_value(&hdr_res->hdr, "Content-Range");
	if (value == NULL)
		return (-1);

	ACL_SAFE_STRNCPY(buf, value, sizeof(buf));
	/* 响应的 Range 数据格式：Content-Range: bytes {range_from}-{range_to}/{total_length}
	 * 其中: {range_from}, {range_to} 的下标是从0开始的
	 */
	/* Content-Range: bytes 2250000-11665200/11665201 */
	/* value: bytes 2250000-11665200/11665201 */
	if (strncasecmp(buf, "bytes", sizeof("bytes") - 1) != 0)
		return (-1);

	ptr = buf + sizeof("bytes") -1;
	while (*ptr == ' ' || *ptr == '\t') {
		ptr++;
	}
	if (*ptr == 0)
		return (-1);

	/* ptr: 2250000-11665200/11665201 */
	pfrom = ptr;
	pto = strchr(pfrom, '-');
	if (pto == NULL || pto == pfrom)
		return (-1);
	*pto++ = 0;
	/* pto: 11665200/11665201 */

	ptr = strchr(pto, '/');
	if (ptr == NULL || ptr == pto)
		return (-1);

	*ptr++ = 0;

	/* pto: 11665200; ptr: 11665201 */

	*range_from = acl_atoi64(pfrom);
	if (*range_from < 0)
		return (-1);

	*range_to = acl_atoi64(pto);
	if (*range_to < 0)
		return (-1);

	/* 可选项 */
	if (total_length) {
		*total_length = acl_atoi64(ptr);
		if (*total_length < 0)
			return (-1);
	}
	return (0);
}
