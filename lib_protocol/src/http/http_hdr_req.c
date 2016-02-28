#include "StdAfx.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stdlib/acl_iterator.h"
#include "http.h"
#include "http/lib_http.h"

static int __http_hdr_max_request = 30;
static int __http_hdr_max_cookies = 30;
static void __get_host_from_url(char *buf, size_t size, const char *url);

static void __hdr_init(HTTP_HDR_REQ *hh)
{
	const char  *myname = "__hdr_init";

	hh->url_part = acl_vstring_alloc(128);
	if (hh->url_part == NULL)
		acl_msg_fatal("%s, %s(%d): alloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
	hh->url_path = acl_vstring_alloc(64);
	if (hh->url_path == NULL)
		acl_msg_fatal("%s, %s(%d): alloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());

	hh->url_params = acl_vstring_alloc(64);
	if (hh->url_params == NULL)
		acl_msg_fatal("%s, %s(%d): alloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());

	hh->file_path = acl_vstring_alloc(256);
	if (hh->file_path == NULL)
		acl_msg_fatal("%s, %s(%d): alloc error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
}

static void __request_args_free_fn(void *arg)
{
	acl_myfree(arg);
}

static void __cookies_args_free_fn(void *arg)
{
	acl_myfree(arg);
}

static void __hdr_free_member(HTTP_HDR_REQ *hh)
{
	if (hh->url_part)
		acl_vstring_free(hh->url_part);
	if (hh->url_path)
		acl_vstring_free(hh->url_path);
	if (hh->url_params)
		acl_vstring_free(hh->url_params);
	if (hh->file_path)
		acl_vstring_free(hh->file_path);
	if (hh->params_table) {
		acl_htable_free(hh->params_table, __request_args_free_fn);
		hh->params_table = NULL;
	}
	if (hh->cookies_table) {
		acl_htable_free(hh->cookies_table, __cookies_args_free_fn);
		hh->cookies_table = NULL;
	}
}

static void __hdr_reset(HTTP_HDR_REQ *hh, int clear_cookies)
{
	hh->port = 80;
	hh->method[0] = 0;
	hh->host[0] = 0;
	hh->flag = 0;

	if (hh->url_part) {
		ACL_VSTRING_RESET(hh->url_part);
		ACL_VSTRING_TERMINATE(hh->url_part);
	}

	if (hh->url_path) {
		ACL_VSTRING_RESET(hh->url_path);
		ACL_VSTRING_TERMINATE(hh->url_path);
	}

	if (hh->url_params) {
		ACL_VSTRING_RESET(hh->url_params);
		ACL_VSTRING_TERMINATE(hh->url_params);
	}

	if (hh->file_path) {
		ACL_VSTRING_RESET(hh->file_path);
		ACL_VSTRING_TERMINATE(hh->file_path);
	}

	if (hh->params_table)
		acl_htable_reset(hh->params_table, __request_args_free_fn);

	if (clear_cookies && hh->cookies_table)
		acl_htable_reset(hh->cookies_table, __cookies_args_free_fn);
}

static void thread_cache_free(ACL_ARRAY *pool)
{
	if ((unsigned long) acl_pthread_self() != acl_main_thread_self())
		acl_array_free(pool, (void (*)(void*)) http_hdr_req_free);
}

static acl_pthread_key_t cache_key = (acl_pthread_key_t) -1;

#ifndef	USE_TLS_EX
static ACL_ARRAY *cache_pool = NULL;
static void main_cache_free(void)
{
	if (cache_pool) {
		acl_array_free(cache_pool, (void (*)(void*)) http_hdr_req_free);
		cache_pool = NULL;
	}
}

static acl_pthread_once_t once_control = ACL_PTHREAD_ONCE_INIT;
static void cache_init(void)
{
	acl_pthread_key_create(&cache_key, (void (*)(void*)) thread_cache_free);
}
#endif

/* 生成一个新的 HTTP_HDR_REQ 数据结构 */

HTTP_HDR_REQ *http_hdr_req_new(void)
{
	HTTP_HDR_REQ *hh;
	ACL_ARRAY *pool;

	if (var_http_tls_cache <= 0) {
		hh = (HTTP_HDR_REQ *) http_hdr_new(sizeof(HTTP_HDR_REQ));
		__hdr_init(hh);
		return hh;
	}

#ifdef	USE_TLS_EX
	pool = (ACL_ARRAY*) acl_pthread_tls_get(&cache_key);
	if (pool == NULL) {
		pool = acl_array_create(100);
		acl_pthread_tls_set(cache_key, pool,
			(void (*)(void*)) thread_cache_free);
	}

	pool = (ACL_ARRAY*) acl_pthread_tls_get(&cache_key);
	hh = (HTTP_HDR_REQ*) pool->pop_back(pool);
	if (hh) {
		__hdr_reset(hh, 1);
		http_hdr_reset((HTTP_HDR *) hh);
		return hh;
	}
#else
	acl_pthread_once(&once_control, cache_init);
	pool = (ACL_ARRAY*) acl_pthread_getspecific(cache_key);
	if (pool == NULL) {
		pool = acl_array_create(100);
		acl_pthread_setspecific(cache_key, pool);
		if ((unsigned long) acl_pthread_self() == acl_main_thread_self()) {
			cache_pool = pool;
			atexit(main_cache_free);
		}
	}
	hh = (HTTP_HDR_REQ*) pool->pop_back(pool);
	if (hh) {
		__hdr_reset(hh, 1);
		http_hdr_reset((HTTP_HDR *) hh);
		return hh;
	}
#endif

	hh = (HTTP_HDR_REQ *) http_hdr_new(sizeof(HTTP_HDR_REQ));
	__hdr_init(hh);
	return hh;
}

HTTP_HDR_REQ *http_hdr_req_create(const char *url,
	const char *method, const char *version)
{
	const char *myname = "http_hdr_req_create";
	HTTP_HDR_REQ *hdr_req;
	ACL_VSTRING *req_line = acl_vstring_alloc(256);
	HTTP_HDR_ENTRY *entry;
	const char *ptr;
	static char *__user_agent = "Mozilla/5.0 (Windows; U; Windows NT 5.0"
		"; zh-CN; rv:1.9.0.3) Gecko/2008092417 ACL/3.0.6";

	if (url == NULL || *url == 0) {
		acl_msg_error("%s(%d): url invalid", myname, __LINE__);
		return NULL;
	}
	if (method == NULL || *method == 0) {
		acl_msg_error("%s(%d): method invalid", myname, __LINE__);
		return NULL;
	}
	if (version == NULL || *version == 0) {
		acl_msg_error("%s(%d): version invalid", myname, __LINE__);
		return NULL;
	}

	acl_vstring_strcpy(req_line, method);
	acl_vstring_strcat(req_line, " ");

	if (strncasecmp(url, "http://", sizeof("http://") - 1) == 0)
		url += sizeof("http://") - 1;
	else if (strncasecmp(url, "https://", sizeof("https://") - 1) == 0)
		url += sizeof("https://") -1;
	ptr = strchr(url, '/');
	if (ptr)
		acl_vstring_strcat(req_line, ptr);
	else {
		ACL_VSTRING_ADDCH(req_line, '/');
		ACL_VSTRING_TERMINATE(req_line);
	}

	acl_vstring_strcat(req_line, " ");
	acl_vstring_strcat(req_line, version);

	entry = http_hdr_entry_new(acl_vstring_str(req_line));
	acl_vstring_free(req_line);

	if (entry == NULL) {
		acl_msg_error("%s(%d): http_hdr_entry_new return null for (%s)",
			myname, __LINE__, acl_vstring_str(req_line));
		return NULL;
	}

	hdr_req = http_hdr_req_new();
	http_hdr_append_entry(&hdr_req->hdr, entry);
	hdr_req->flag |= (HTTP_HDR_REQ_FLAG_PARSE_PARAMS | HTTP_HDR_REQ_FLAG_PARSE_COOKIE);
	if (http_hdr_req_line_parse(hdr_req) < 0) {
		http_hdr_req_free(hdr_req);
		return NULL;
	}

	hdr_req->host[0] = 0;
	__get_host_from_url(hdr_req->host, sizeof(hdr_req->host), url);
	if (hdr_req->host[0] != 0)
		http_hdr_put_str(&hdr_req->hdr, "Host", hdr_req->host);
	http_hdr_put_str(&hdr_req->hdr, "Connection", "Close");
	http_hdr_put_str(&hdr_req->hdr, "User-Agent", __user_agent);

	return hdr_req;
}

static void clone_table_entry(ACL_HTABLE_INFO *info, void *arg)
{
	const char *myname = "clone_table_entry";
	ACL_HTABLE *table = (ACL_HTABLE*) arg;
	char *value;

	value = acl_mystrdup(info->value);

	if (acl_htable_enter(table, info->key.key, value) == NULL)
		acl_msg_fatal("%s, %s(%d): acl_htable_enter error=%s",
			__FILE__, myname, __LINE__, acl_last_serror());
}

HTTP_HDR_REQ *http_hdr_req_clone(const HTTP_HDR_REQ* hdr_req)
{
	HTTP_HDR_REQ *hh;

	hh = http_hdr_req_new();
	http_hdr_clone(&hdr_req->hdr, &hh->hdr);

	hh->port = hdr_req->port;
	ACL_SAFE_STRNCPY(hh->method, hdr_req->method, sizeof(hh->method));
	ACL_SAFE_STRNCPY(hh->host, hdr_req->host, sizeof(hh->host));
	acl_vstring_strcpy(hh->url_part, acl_vstring_str(hdr_req->url_part));
	acl_vstring_strcpy(hh->url_path, acl_vstring_str(hdr_req->url_path));
	acl_vstring_strcpy(hh->url_params, acl_vstring_str(hdr_req->url_params));
	acl_vstring_strcpy(hh->file_path, acl_vstring_str(hdr_req->file_path));

	if (hdr_req->params_table) {
		hh->params_table = acl_htable_create(__http_hdr_max_request, 0);
		acl_htable_walk(hdr_req->params_table, clone_table_entry,
				(void*) hh->params_table);
	}
	if (hdr_req->cookies_table) {
		hh->cookies_table = acl_htable_create(__http_hdr_max_cookies, 0);
		acl_htable_walk(hdr_req->cookies_table, clone_table_entry,
				(void*) hh->cookies_table);
	}

	return hh;
}

/* 释放一个 HTTP_HDR_REQ 结构 */

void http_hdr_req_free(HTTP_HDR_REQ *hh)
{
	ACL_ARRAY *pool;

	if (hh == NULL)
		return;

	if (var_http_tls_cache <= 0 || cache_pool == NULL) {
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

void http_hdr_req_reset(HTTP_HDR_REQ *hh)
{
	if (hh == NULL)
		return;

	http_hdr_reset((HTTP_HDR *) hh);
	__hdr_free_member(hh);
	__hdr_init(hh);
}

/*----------------------------------------------------------------------------*/

/* 将 cookie 行中的 name=value 对置入哈希表中, 以便于查询 */

static void __add_cookie_item(ACL_HTABLE *table, const char *data)
{
/* data format: name=value */
	const char *myname = "__add_cookie_item";
	ACL_ARGV *argv = NULL;
	ACL_VSTRING *str = NULL;
	const char *name;
	char *value;
	char *ptr;
	int   i;

#undef	RETURN
#define	RETURN do {  \
	if (argv)  \
		acl_argv_free(argv);  \
	return;  \
} while(0);

#undef	TRUNC_BLANK
#define	TRUNC_BLANK(_x_) do {  \
	char *_ptr_;  \
	while(*_x_ == ' ' || *_x_ == '\t')  \
		_x_++;  \
	if (*_x_ == 0)  \
		RETURN;  \
	_ptr_ = _x_;  \
	while (*_ptr_) {  \
		if (*_ptr_ == ' ' || *_ptr_ == '\t') {  \
			*_ptr_ = 0;  \
			break;  \
		}  \
		_ptr_++;  \
	}  \
} while (0);

#undef	TRUNC_BLANK_NORETURN
#define	TRUNC_BLANK_NORETURN(_x_) do {  \
	char *_ptr_;  \
	while(*_x_ == ' ' || *_x_ == '\t')  \
		_x_++;  \
	_ptr_ = _x_;  \
	while (*_ptr_) {  \
		if (*_ptr_ == ' ' || *_ptr_ == '\t') {  \
			*_ptr_ = 0;  \
			break;  \
		}  \
		_ptr_++;  \
	}  \
} while (0);

	argv = acl_argv_split(data, "=");
	if (argv->argc < 2)   /* data: "name" or "name="*/
		RETURN;

	ptr = acl_argv_index(argv, 0);
	TRUNC_BLANK(ptr);
	name = ptr;

	/* 有些站点的COOKIE比较弱，如和讯的reg.hexun.com，COOKIE名会有重复
	 * 的情况，所以必须判断一下，不必重复存储相同名字的COOKIE值，即如果
	 * 出现重复COOKIE名，则只存储第一个，这样就避免了采用哈希方式存储的
	 * 漏内存的现象发生。--- zsx, 2008.1.8
	 */
	if (acl_htable_find(table, name) != NULL) {
		RETURN;
	}

	str = acl_vstring_alloc(256);

	for (i = 1; i < argv->argc; i++) {
		ptr = acl_argv_index(argv, i);
		if (ptr == NULL)
			break;
		TRUNC_BLANK_NORETURN(ptr);
		if (*ptr == 0)
			continue;
		if (i == 1)
			acl_vstring_sprintf_append(str, "%s", ptr);
		else
			acl_vstring_sprintf_append(str, "=%s", ptr);
	}

	/* 将真实的存储数据的区域内存引出, 同时将外包结构内存释放,
	 * POSTFIX真是个好东西:) ---zsx
	 */
	value = acl_vstring_export(str);

	if (acl_htable_enter(table, name, value) == NULL)
		acl_msg_fatal("%s, %s(%d): acl_htable_enter error=%s",
			__FILE__, myname, __LINE__, acl_last_serror());

	RETURN;
}

int http_hdr_req_cookies_parse(HTTP_HDR_REQ *hh)
{
	/* data format: "name1=value1; name2=value2; name3=value3" */
	const char  *myname = "http_hdr_req_cookies_parse";
	const HTTP_HDR_ENTRY *entry;
	ACL_ARGV *argv;
	const char *ptr;
	ACL_ITER iter;

	if (hh == NULL)
		acl_msg_fatal("%s, %s(%d): input invalid",
			__FILE__, myname, __LINE__);
	if ((hh->flag & HTTP_HDR_REQ_FLAG_PARSE_COOKIE) == 0)
		return 0;

	entry = http_hdr_entry((HTTP_HDR *) hh, "Cookie");
	if (entry == NULL)
		return 0;

	/* bugfix: 在创建哈希表时此处不应设置 ACL_HTABLE_FLAG_KEY_REUSE 标志位，
	 * 如果设置了此标志，则在 __add_cookie_item 中调用 acl_htable_enter 时，
	 * 会将 name 值的内存交给 htable，但随后在宏 RETURN 时却调用了释放数组
	 * 的函数 acl_argv_free(argv)，将 name 所属的数组内存一起给释放了
	 * ---zsx, 2014.5.13
	 */
	if (hh->cookies_table == NULL)
#if 0
		hh->cookies_table = acl_htable_create(__http_hdr_max_cookies,
			ACL_HTABLE_FLAG_KEY_REUSE);
#else
		hh->cookies_table = acl_htable_create(__http_hdr_max_cookies, 0);
#endif

	if (hh->cookies_table == NULL)
		acl_msg_fatal("%s, %s(%d): htable create error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());

	/* 分隔数据段 */
	argv = acl_argv_split(entry->value, ";");
	acl_foreach(iter, argv) {
		ptr = (const char*) iter.data;
		if (ptr && *ptr)
			__add_cookie_item(hh->cookies_table, ptr);
	}
	acl_argv_free(argv);
	return 0;
}

const char *http_hdr_req_cookie_get(HTTP_HDR_REQ *hh, const char *name)
{
	const char *myname = "http_hdr_req_cookie_get";

	if (hh == NULL || name == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
				__FILE__, myname, __LINE__);
		return NULL;
	}

	if (hh->cookies_table == NULL) {
		acl_msg_warn("%s, %s(%d): cookies_table null",
				__FILE__, myname, __LINE__);
		return NULL;
	}

	return acl_htable_find(hh->cookies_table, name);
}

/*--------------- 分析HTTP协议请求头中第一行数据信息的函数集合   -------------*/

/* 将HTTP请求行中的 name=value 对置入哈希表中, 以便于查询 */
static void __add_request_item(ACL_HTABLE *table, const char *data)
{
	/* data format: name=value */
	const char *myname = "__add_request_item";
	ACL_ARGV *argv;
	const char *name;
	char *value;

	argv = acl_argv_split(data, "=");
	if (argv->argc != 2) {
		acl_argv_free(argv);
		return;
	}

	name = acl_argv_index(argv, 0);

	/* 先判断是否已经存在该变量名，防止出现重复现象，
	 * 避免采用哈希存储时的内存泄漏 ---zsx, 2008.1.8
	 */
	if (acl_htable_find(table, name) != NULL) {
		acl_argv_free(argv);  /* bugfix: add by zsx, 2009-11-11 */
		return;
	}

	value = acl_mystrdup(acl_argv_index(argv, 1));
	if (value == NULL)
		acl_msg_fatal("%s, %s(%d): strdup error=%s", __FILE__, myname,
			__LINE__, acl_last_serror());

	if (acl_htable_enter(table, name, value) == NULL)
		acl_msg_error("%s, %s(%d): acl_htable_enter error=%s", __FILE__,
			myname, __LINE__, acl_last_serror());

	acl_argv_free(argv);
}

/* 如果没有 Host 实体，则需要从首行的URL分析以期获得Host */

/* data: "http://www.gmail.com:443/path/test.cgi?name=value&name2=value2 */

static void __get_host_from_url(char *buf, size_t size, const char *url)
{
	const char *ptr1, *ptr2;
	size_t n;

	buf[0] = 0;
	if (strncasecmp(url, "http://", sizeof("http://") - 1) == 0)
		ptr1 = url + sizeof("http://") - 1;
	else if (strncasecmp(url, "https://", sizeof("https://") - 1) == 0)
		ptr1 = url + sizeof("https://") - 1;
	else
		ptr1 = url;

	if (ptr1 == NULL || *ptr1 == 0 || *ptr1 == '/')
		return;

	ptr2 = strchr(ptr1, '/');
	if (ptr2)
		n = ptr2 - ptr1;
	else
		n = strlen(ptr1);

	n++;

	if (n > size)
		n = size;

	ACL_SAFE_STRNCPY(buf, ptr1, (int) n);
}

static void __strip_url_path(ACL_VSTRING *buf, const char *url)
{
	ACL_ARGV *argv;
	const char *ptr;
	const char  last_ch = *(url + strlen(url) - 1);
	ACL_ITER iter;

	argv = acl_argv_split(url, "/");

	/* xxx: 必须将下面两行的初始化放在 acl_argv_split 的后面，因为 url
	 * 所指内容有可能与 buf 中的缓冲区地址相同，参见 __strip_url_path
	 * 的两处调用；另外，在调用 ACL_VSTRING_RESET 后还必须调用
	 * ACL_VSTRING_TERMINATE，否则 ACL_VSTRING_RESET 仅移动指针位置，
	 * 并不会将初始位置赋 '\0'
	 */
	ACL_VSTRING_RESET(buf);
	ACL_VSTRING_TERMINATE(buf);

	acl_foreach(iter, argv) {
		ptr = (const char*) iter.data;
		if (strcmp(ptr, ".") == 0 || strcmp(ptr, "..") == 0)
			continue;
		ACL_VSTRING_ADDCH(buf, '/');
		acl_vstring_strcat(buf, ptr);
	}

	/* make the last char is ok */
	if (last_ch == '/') {
		ACL_VSTRING_ADDCH(buf, '/');
		ACL_VSTRING_TERMINATE(buf);
	}

	acl_argv_free(argv);
}

static void __parse_url_and_port(HTTP_HDR_REQ *hh, const char *url)
{
	const char *myname = "__parse_url_and_port";
	int   i;
	ACL_ARGV *url_argv;
	const char *ptr;

	hh->port = 80;  /* set the default server port */
	ptr = strchr(hh->host, ':');
	if (ptr) {
		ptr++;
		hh->port = atoi(ptr);
	}

	if (strncasecmp(url, "http://", sizeof("http://") - 1) == 0)
		url += sizeof("http://") - 1;
	else if (strncasecmp(url, "https://", sizeof("https://") - 1) == 0) {
		url += sizeof("https://") - 1;
		hh->port = 443;  /* set the default https server port */
	}

	if (strcasecmp(hh->method, "CONNECT") == 0) {
		ptr = strchr(url, ':');
		if (ptr) {
			ptr++;
			hh->port = atoi(ptr);
		}
	}

	/* sanity check */
	if (hh->port <= 0)
		hh->port = 80;

	if (*url == '/')
		acl_vstring_strcpy(hh->url_part, url);
	else if ((url = strchr(url, '/')) == NULL) {
		ACL_VSTRING_ADDCH(hh->url_part, '/');
		ACL_VSTRING_TERMINATE(hh->url_part);
		ACL_VSTRING_ADDCH(hh->url_path, '/');
		ACL_VSTRING_TERMINATE(hh->url_path);
		return;
	} else {
		acl_vstring_strcpy(hh->url_part, url);
		url++;
	}

	/* get url_path and url_params */
	ptr = strchr(url, '?');
	if (ptr == NULL)
		__strip_url_path(hh->url_path, url);
	else {
		acl_vstring_strncpy(hh->url_path, url, ptr - url);
		__strip_url_path(hh->url_path, acl_vstring_str(hh->url_path));
		ptr++;  /* skip '?' */
		if (*ptr)
			acl_vstring_strcpy(hh->url_params, ptr);
	}

	if ((hh->flag & HTTP_HDR_REQ_FLAG_PARSE_PARAMS) == 0)
		return;
	if (ACL_VSTRING_LEN(hh->url_params) == 0)
		return;
	if (hh->params_table == NULL)
		hh->params_table = acl_htable_create(__http_hdr_max_request, 0);
	if (hh->params_table == NULL)
		acl_msg_fatal("%s, %s(%d): htable create error(%s)",
			__FILE__, myname, __LINE__, acl_last_serror());
	url_argv = acl_argv_split(acl_vstring_str(hh->url_params), "&");
	for (i = 0; i < url_argv->argc; i++) {
		ptr = acl_argv_index(url_argv, i);
		if (ptr == NULL)
			break;
		__add_request_item(hh->params_table, ptr);
	}
	acl_argv_free(url_argv);
}

int http_hdr_req_line_parse(HTTP_HDR_REQ *hh)
{
	const char *myname = "http_hdr_req_line_parse";
	ACL_ARGV *request_argv;
	HTTP_HDR_ENTRY *entry;
	char *purl;
	const char *ptr;
	int   ret;

	if (hh == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
				__FILE__, myname, __LINE__);
		return -1;
	}
	if (hh->hdr.entry_lnk == NULL)
		acl_msg_fatal("%s, %s(%d): entry_lnk null",
				__FILE__, myname, __LINE__);

	if (acl_array_size(hh->hdr.entry_lnk) <= 0) {
		acl_msg_error("%s, %s(%d): no method",
				__FILE__, myname, __LINE__);
		return -1;
	}

	entry = (HTTP_HDR_ENTRY *) acl_array_index(hh->hdr.entry_lnk, 0);
	if (entry == NULL) {
		acl_msg_error("%s, %s(%d): null array",
				__FILE__, myname, __LINE__);
		return -1;
	}

	if (entry->value == NULL || *(entry->value) == 0) {
		acl_msg_error("%s, %s(%d): entry->value invalid",
				__FILE__, myname, __LINE__);
		return -1;
	}

#if 0
	/* 去掉该判断，以允许上层应用可以扩展 HTTP 请求方法 */
	if (strcasecmp(entry->name, "POST") != 0
	    && strcasecmp(entry->name, "GET") != 0
	    && strcasecmp(entry->name, "CONNECT") != 0
	    && strcasecmp(entry->name, "HEAD") != 0)
	{
		acl_msg_error("%s, %s(%d): invalid http method=%s",
				__FILE__, myname, __LINE__,
				entry->name);
		return -1;

	}
#endif

	ACL_SAFE_STRNCPY(hh->method, entry->name, sizeof(hh->method));

	/* data: "/path/test.cgi?name=value&name2=value2 HTTP/1.0"
	 * or: "http://www.test.com/path/test.cgi?name=value&name2=value2 HTTP/1.0"
	 */
	request_argv = acl_argv_split(entry->value, "\t ");
	if (request_argv->argc != 2) {
		acl_msg_error("%s, %s(%d): invalid request line=%s, argc=%d",
			__FILE__, myname, __LINE__,
			entry->value, request_argv->argc);
		acl_argv_free(request_argv);
		return -1;
	}

	/* data[0]: "/path/test.cgi?name=value&name2=value2"
	 *      or: "http://www.test.com/path/test.cgi?name=value&name2=value2"
	 * data[1]: "HTTP/1.0"
	 */ 

	purl = acl_argv_index(request_argv, 0);

	/* get HOST item */
	ptr = http_hdr_entry_value(&hh->hdr, "Host");
	if (ptr)
		ACL_SAFE_STRNCPY(hh->host, ptr, sizeof(hh->host));
	else
		__get_host_from_url(hh->host, sizeof(hh->host), purl);

	/* parse the first line's url and get server side's port */
	__parse_url_and_port(hh, purl);

	ptr = acl_argv_index(request_argv, 1);

	ret = http_hdr_parse_version(&hh->hdr, ptr);

	acl_argv_free(request_argv);

	return ret;
}

int http_hdr_req_parse(HTTP_HDR_REQ *hh)
{
	return http_hdr_req_parse3(hh, 1, 1);
}

int http_hdr_req_parse3(HTTP_HDR_REQ *hh, int parse_params, int parse_cookie)
{
	if (parse_params)
		hh->flag |= HTTP_HDR_REQ_FLAG_PARSE_PARAMS;
	if (http_hdr_req_line_parse(hh) < 0)
		return -1;
	if (parse_cookie) {
		hh->flag |= HTTP_HDR_REQ_FLAG_PARSE_COOKIE;
		if (http_hdr_req_cookies_parse(hh) < 0)
			return -1;
	}
	return http_hdr_parse(&hh->hdr);
}

int http_hdr_req_rewrite2(HTTP_HDR_REQ *hh, const char *url)
{
	const char *myname = "http_hdr_req_rewrite2";
	HTTP_HDR_ENTRY *first_entry, *entry;
	ACL_VSTRING *buf;
	char  host[256], *ptr;
	const char *phost, *purl;
	int   i, n;

	if (hh->hdr.entry_lnk == NULL)
		acl_msg_fatal("%s(%d): entry_lnk null", myname, __LINE__);

	n = acl_array_size(hh->hdr.entry_lnk);
	if (n <= 0) {
		acl_msg_error("%s(%d): first entry null", myname, __LINE__);
		return -1;
	}

	/* url: http[s]://domain[/path?params], /[path?params] */
	if (strncasecmp(url, "http://", strlen("http://")) == 0) {
		phost = url + strlen("http://");
		purl = strchr(phost, '/');
		if (purl == NULL)
			purl = "/";
	} else if (strncasecmp(url, "https://", strlen("https://")) == 0) {
		phost = url + strlen("https://");
		purl = strchr(phost, '/');
		if (purl == NULL)
			purl = "/";
	} else {
		phost = hh->host;  /* 如果URL中没有 http[s]:// 则默认采用原 Host 字段 */
		purl = url;
	}
	host[0] = 0;
	ACL_SAFE_STRNCPY(host, phost, sizeof(host));
	ptr = strchr(host, '/');
	if (ptr)
		*ptr = 0;

	/* 将新的主机信息覆盖旧信息 */
	ACL_SAFE_STRNCPY(hh->host, host, sizeof(hh->host));

	buf = acl_vstring_alloc(256);
	if (phost == hh->host) {
		if (*url == '/')
			acl_vstring_sprintf(buf, "%s HTTP/%d.%d",
				purl, hh->hdr.version.major, hh->hdr.version.minor);
		else
			acl_vstring_sprintf(buf, "/%s HTTP/%d.%d",
				purl, hh->hdr.version.major, hh->hdr.version.minor);
	} else {
		acl_vstring_sprintf(buf, "%s HTTP/%d.%d",
			purl, hh->hdr.version.major, hh->hdr.version.minor);
	}
	first_entry = (HTTP_HDR_ENTRY *) acl_array_index(hh->hdr.entry_lnk, 0);
	if (first_entry == NULL || first_entry->value == NULL)
		acl_msg_fatal("%s(%d): first_entry invalid", myname, __LINE__);
	acl_myfree(first_entry->value);
	first_entry->value = acl_vstring_export(buf);
	__hdr_reset(hh, 0);

	if (host[0] != 0) {
		for (i = 1; i < n; i++) {
			entry = (HTTP_HDR_ENTRY*) acl_array_index(hh->hdr.entry_lnk, i);
			if (strcasecmp(entry->name, "host") == 0) {
				acl_myfree(entry->value);
				entry->value = acl_mystrdup(host);
				break;
			}
		}
	}

	hh->flag |= (HTTP_HDR_REQ_FLAG_PARSE_PARAMS | HTTP_HDR_REQ_FLAG_PARSE_COOKIE);
	if (http_hdr_req_line_parse(hh) < 0)
		return (-1);
	return 0;
}

HTTP_HDR_REQ *http_hdr_req_rewrite(const HTTP_HDR_REQ *hh, const char *url)
{
	HTTP_HDR_REQ *hdr_req = http_hdr_req_clone(hh);

	if (http_hdr_req_rewrite2(hdr_req, url) < 0) {
		http_hdr_req_free(hdr_req);
		return NULL;
	}
	return hdr_req;
}

/* 取得HTTP请求的方法 */

const char *http_hdr_req_method(HTTP_HDR_REQ *hh)
{
	return hh->method;
}

/* 获取请求URL中某个请求字段的数据, 如取: /cgi-bin/n1=v1&n2=v2 中的 n2的值v2 */

const char *http_hdr_req_param(HTTP_HDR_REQ *hh, const char *name)
{
	const char *myname = "http_hdr_req_get";

	if (hh == NULL || name == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
				__FILE__, myname, __LINE__);
		return NULL;
	}

	if (hh->params_table == NULL)
		return NULL;

	return acl_htable_find(hh->params_table, name);
}

const char *http_hdr_req_url_part(HTTP_HDR_REQ *hh)
{
	const char *myname = "http_hdr_req_url_part";

	if (hh == NULL) {
		acl_msg_error("%s, %s(%d): input invalid",
				__FILE__, myname, __LINE__);
		return NULL;
	}

	if (ACL_VSTRING_LEN(hh->url_part) == 0)
		return NULL;
	return acl_vstring_str(hh->url_part);
}

const char *http_hdr_req_url_path(HTTP_HDR_REQ *hh)
{
	if (ACL_VSTRING_LEN(hh->url_path) == 0)
		return NULL;

	return acl_vstring_str(hh->url_path);
}

const char *http_hdr_req_host(HTTP_HDR_REQ *hh)
{
	if (hh->host[0] != 0)
		return hh->host;
	else
		return NULL;
}

static void free_vstring(ACL_VSTRING *buf)
{
	acl_vstring_free(buf);
}

const char *http_hdr_req_url(HTTP_HDR_REQ *hh)
{
	static acl_pthread_key_t key = (acl_pthread_key_t) -1;
	ACL_VSTRING *buf;

	buf = acl_pthread_tls_get(&key);
	if (buf == NULL) {
		buf = acl_vstring_alloc(256);
		acl_pthread_tls_set(key, buf, (void (*)(void*)) free_vstring);
	}

	acl_vstring_strcpy(buf, hh->host);
	acl_vstring_strcat(buf, acl_vstring_str(hh->url_part));
	return acl_vstring_str(buf);
}

int http_hdr_req_range(HTTP_HDR_REQ *hdr_req, http_off_t *range_from,
	http_off_t *range_to)
{
	const char *myname = "http_hdr_req_range";
	char  buf[256], *ptr1;
	const char *ptr;

	if (range_from == NULL)
		acl_msg_fatal("%s(%d): range_from null", myname, __LINE__);
	if (range_to == NULL)
		acl_msg_fatal("%s(%d): range_to null", myname, __LINE__);

	/* 数据格式: Range: bytes={range_from}-{range_to}
	 * 或: Range: bytes={range_from}-
	 */
	ptr = http_hdr_entry_value(&hdr_req->hdr, "Range");
	if (ptr == NULL)
		return -1;
	ptr = strstr(ptr, "bytes=");
	if (ptr == NULL)
		return -1;
	ptr += strlen("bytes=");
	ACL_SAFE_STRNCPY(buf, ptr, sizeof(buf));
	ptr1 = buf;
	while (*ptr1) {
		if (*ptr1 == '-' || *ptr1 == ' ') {
			*ptr1 = 0;
			*range_from = acl_atoi64(buf);
			if (*range_from < 0)
				return (-1);
			if (*(++ptr1) == 0)
				*range_to = -1;
			else
				*range_to = acl_atoi64(ptr1);
			if (*range_to <= 0)
				*range_to = -1;
			return 0;
		}
		ptr1++;
	}

	return -1;
}
