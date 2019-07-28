
#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include "lib_protocol.h"
#include "dict_pool.h"
#include "service_main.h"
#include "http_service.h"

#define	STR	acl_vstring_str
#define	LEN	ACL_VSTRING_LEN
#define	MAX_LEN	(1024 * 8)

static ACL_HTABLE *__db_table = NULL;

static const char *__partions[] = {
        "./cache1",     
        "./cache2",             
        "./cache3",     
        "./cache4"
};                      
static int   __partions_size = 4;

/* forward declare */
static void http_service_start(HTTP_CLIENT *http_client);

void http_service_init(void *init_ctx acl_unused)
{
	const char *myname = "http_service_init";
	DICT_POOL *pool;
	ACL_ARGV *argv;
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	char *ptr, *name;
	int   i, n, has_default_db = 0;

	acl_init();
	http_init(NULL);
	dict_pool_init();

	if (__db_table != NULL)
		acl_msg_fatal("%s(%d): service_init been called more than once",
			myname, __LINE__);

	__db_table = acl_htable_create(10);

	argv = acl_argv_split(var_cfg_dbnames, ", ");
	if (argv == NULL)
		acl_msg_fatal("%s(%d): db_names(%s) invalid", var_cfg_dbnames);

	for (i = 0; i < argv->argc; i++) {
		name = argv->argv[i];
		ptr = strchr(name, ':');
		if (ptr == NULL) {
			acl_msg_warn("%s(%d): dbname(%s) use one db", myname, __LINE__, name);
			n = 1;
		} else {
			*ptr++ = 0;
			n = atoi(ptr);
		}
		if (n <= 0)
			n = 1;
		acl_lowercase(name);
		acl_vstring_sprintf(buf, "btree:%s/%s", var_cfg_dbpath, name);
		pool = dict_pool_new(__partions, __partions_size, "btree",
				var_cfg_dbpath, name, 1);
		if (acl_htable_enter(__db_table, name, (char*) pool) == NULL)
			acl_msg_fatal("%s(%d): add %s error", myname, __LINE__, name);
		if (strcmp(name, "default") == 0)
			has_default_db = 1;
	}
	acl_argv_free(argv);

	if (!has_default_db) {
		acl_vstring_sprintf(buf, "btree:%s/default", var_cfg_dbpath);
		pool = dict_pool_new(__partions, __partions_size, "btree",
				var_cfg_dbpath, "default", 1);
		if (acl_htable_enter(__db_table, "default", (char*) pool) == NULL)
			acl_msg_fatal("%s(%d): add default error", myname, __LINE__);
	}

	acl_vstring_free(buf);
}

static void dict_pool_free_fn(char *arg)
{
	DICT_POOL *pool = (DICT_POOL*) arg;

	dict_pool_free(pool);
}

void http_service_exit(void *exit_ctx acl_unused)
{
	if (__db_table)
		acl_htable_free(__db_table, dict_pool_free_fn);
}

static int send_ready(ACL_ASTREAM *stream acl_unused, void *context)
{
	HTTP_CLIENT *http_client = (HTTP_CLIENT*) context;
	int   keep_alive;

	TRACE();
	if (http_client->hdr_req && http_client->hdr_req->hdr.keep_alive)
		keep_alive = 1;
	else
		keep_alive = 0;

	http_client_reset(http_client);

	/* get next request */
	if (keep_alive)
		http_service_start(http_client);
	return (0);
}

static int set_to_db(HTTP_CLIENT *http_client, DICT_POOL *dict_pool,
	const char *key, const char *value, size_t len)
{
	static const char reply_200_keep[] = "HTTP/1.1 200 OK\r\n"
		"Accept-Ranges: bytes\r\n"
		"Server: dict_http/1.0.0 (Unix)\r\n"
		"Content-Length: 9\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"200 OK!\r\n";
	static const char reply_200_close[] = "HTTP/1.1 200 OK\r\n"
		"Accept-Ranges: bytes\r\n"
		"Server: dict_http/1.0.0 (Unix)\r\n"
		"Content-Length: 9\r\n"
		"Connection: close\r\n"
		"\r\n"
		"200 OK!\r\n";

	dict_pool_set(dict_pool, (char*) key, strlen(key), (char*) value, len);

	acl_aio_ctl(http_client->stream,
		ACL_AIO_CTL_WRITE_HOOK_ADD, send_ready, http_client,
		ACL_AIO_CTL_END);
	if (http_client->hdr_req->hdr.keep_alive) {
		acl_aio_writen(http_client->stream, reply_200_keep, sizeof(reply_200_keep) - 1);
		return (0);
	} else {
		acl_aio_writen(http_client->stream, reply_200_close, sizeof(reply_200_close) - 1);
		return (-1);
	}
}

static int get_from_db(HTTP_CLIENT *http_client, DICT_POOL *dict_pool, const char *key)
{
	static char reply_404_close[] = "HTTP/1.1 404 not found\r\n"
		"Accept-Ranges: bytes\r\n"
		"Server: dict_http/1.0.0 (Unix)\r\n"
		"Content-Length: 15\r\n"
		"Connection: close\r\n"
		"\r\n"
		"404 not found\r\n";
	static char reply_404_keep[] = "HTTP/1.1 404 not found\r\n"
		"Accept-Ranges: bytes\r\n"
		"Server: dict_http/1.0.0 (Unix)\r\n"
		"Content-Length: 15\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"404 not found\r\n";
	static char reply_200_close[] = "HTTP/1.1 200 OK\r\n"
		"Accept-Ranges: bytes\r\n"
		"Server: dict_http/1.0.0 (Unix)\r\n"
		"Content-type: text/html\r\n"
		"Connection: close\r\n";
	static char reply_200_keep[] = "HTTP/1.1 200 OK\r\n"
		"Accept-Ranges: bytes\r\n"
		"Server: dict_http/1.0.0 (Unix)\r\n"
		"Content-type: text/html\r\n"
		"Connection: Keep-Alive\r\n";
	static const char *reply_length_fmt = "Content-Length: %d\r\n\r\n";
	char  found_second[256];
	char *value;
	size_t   size;
	struct iovec iov[3];

	if (var_cfg_use_bdb)
		value = dict_pool_get(dict_pool, (char*) key, strlen(key), &size);
	else {
		key = key;
		dict_pool = dict_pool;
		value = acl_mystrdup("test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4\r\n");
		size = strlen(value);
	}

	acl_aio_ctl(http_client->stream,
		ACL_AIO_CTL_WRITE_HOOK_ADD, send_ready, http_client,
		ACL_AIO_CTL_END);

	if (value == NULL) {
		if (http_client->hdr_req->hdr.keep_alive) {
			acl_aio_writen(http_client->stream, reply_404_keep, sizeof(reply_404_keep) - 1);
			return (0);
		} else {
			acl_aio_writen(http_client->stream, reply_404_close, sizeof(reply_404_close) - 1);
			return (-1);
		}
	}

	snprintf(found_second, sizeof(found_second), reply_length_fmt, size);
	if (http_client->hdr_req->hdr.keep_alive) {
		iov[0].iov_base = reply_200_keep;
		iov[0].iov_len  = sizeof(reply_200_keep) - 1;
	} else {
		iov[0].iov_base = reply_200_close;
		iov[0].iov_len  = sizeof(reply_200_close) - 1;
	}
	iov[1].iov_base = found_second;
	iov[1].iov_len  = strlen(found_second);
	iov[2].iov_base = value;
	iov[2].iov_len  = size;

	if (http_client->hdr_req->hdr.keep_alive) {
		TRACE();
		acl_aio_writev(http_client->stream, iov, 3);
		acl_myfree(value);
		return (0);
	} else {
		TRACE();
		acl_aio_writev(http_client->stream, iov, 3);
		acl_myfree(value);
		return (-1);
	}
}

static int read_request_body_ready(int status, const char *data, int dlen, void *arg)
{
	HTTP_CLIENT *http_client = (HTTP_CLIENT*) arg;

	if (status >= HTTP_CHAT_ERR_MIN) {
		acl_msg_error("%s(%d): status(%d) error",
			__FUNCTION__, __LINE__, status);
		http_error_reply(http_client, 400, "Invalid request");
		return (-1);
	} else if (status == HTTP_CHAT_OK) {
		return (set_to_db(http_client,
				http_client->dict_pool,
				STR(http_client->key),
				STR(http_client->sbuf),
				LEN(http_client->sbuf)));
	}

	acl_vstring_memcat(http_client->sbuf, data, dlen);
	if (LEN(http_client->sbuf) >= MAX_LEN) {
		acl_msg_error("%s(%d): len(%s) too long",
			__FUNCTION__, __LINE__, LEN(http_client->sbuf));
		http_error_reply(http_client, 403, "Request too large");
		return (-1);
	}
	return (0);
}

static int get_req_body(HTTP_CLIENT *http_client, DICT_POOL *dict_pool, const char *key)
{
	http_client->dict_pool = dict_pool;
	if (http_client->key == NULL)
		http_client->key = acl_vstring_alloc(256);
	acl_vstring_strcpy(http_client->key, key);

	TRACE();
	if (http_client->http_req != NULL)
		acl_msg_fatal("%s(%d): http_req not null", __FUNCTION__, __LINE__);

	http_client->http_req = http_req_new(http_client->hdr_req);
	http_req_body_get_async(http_client->http_req,
			http_client->stream,
			read_request_body_ready,
			http_client,
			var_cfg_rw_timeout);
	return (0);
}

static int http_service(HTTP_CLIENT *http_client)
{
	const char *myname = "http_service";
	char  dbname[256];
	const char *ptr, *action, *method, *key;
	DICT_POOL *dict_pool;
	HTTP_HDR_REQ *hdr_req = http_client->hdr_req;

	ptr = http_hdr_req_param(hdr_req, "dbname");
	if (ptr == NULL)
		ACL_SAFE_STRNCPY(dbname, "default", sizeof(dbname));
	else {
		ACL_SAFE_STRNCPY(dbname, ptr, sizeof(dbname));
		acl_lowercase(dbname);
	}

	dict_pool = (DICT_POOL*) acl_htable_find(__db_table, dbname);
	if (dict_pool == NULL) {
		acl_msg_error("%s(%d): dbname(%s) not found",
			myname, __LINE__, dbname);
		http_error_reply(http_client, 404, "Dict not exist");
		return (-1);
	}

	method = http_hdr_req_method(hdr_req);
	if (method == NULL) {
		acl_msg_error("%s(%d): no method", myname, __LINE__);
		http_error_reply(http_client, 400, "Bad request, no Method");
		return (-1);
	}

	key = http_hdr_req_param(hdr_req, "key");
	if (key == NULL) {
		acl_msg_error("%s(%d): no key(%s)", myname, __LINE__, STR(hdr_req->url_part));
		http_error_reply(http_client, 400, "Bad request, no key");
		return (-1);
	}

	action = http_hdr_req_param(hdr_req, "action");
	if (action == NULL) {
		acl_msg_error("%s(%d): no action", myname, __LINE__);
		http_error_reply(http_client, 400, "Bad request, no action");
		return (-1);
	} else if (strcasecmp(action, "set") == 0) {
		if (strcasecmp(method, "GET") == 0) {
			ptr = http_hdr_req_param(hdr_req, "value");
			if (ptr == NULL) {
				acl_msg_error("%s(%d): no value", myname, __LINE__);
				http_error_reply(http_client, 400, "Bad request, no value");
				return (-1);
			}
			return (set_to_db(http_client, dict_pool, key, ptr, strlen(ptr)));
		} else if (strcasecmp(method, "POST") == 0) {
			if (http_client->sbuf == NULL)
				http_client->sbuf = acl_vstring_alloc(MAX_LEN);
			return (get_req_body(http_client, dict_pool, key));
		} else {
			acl_msg_error("%s(%d): method(%s) not support",
				myname, __LINE__, method);
			http_error_reply(http_client, 501 , "Method not support");
			return (-1);
		}
	} else if (strcasecmp(action, "get") == 0) {
		if (strcasecmp(method, "GET") != 0) {
			acl_msg_error("%s(%d): not GET method(%s) for get data",
				myname, __LINE__, method);
			http_error_reply(http_client, 400, "Bad request, should be GET request");
			return (-1);
		}
		return (get_from_db(http_client, dict_pool, key));
	} else {
		acl_msg_error("%s(%d): unsupport action(%s)",
			myname, __LINE__, action);
		http_error_reply(http_client, 400, "Bad request, not support action");
		return (-1);
	}
}

static int request_header_ready(int status, void *arg)
{
	HTTP_CLIENT *http_client = (HTTP_CLIENT*) arg;

	if (status != HTTP_CHAT_OK) {
		TRACE();
		acl_debug(20, 2) ("%s(%d): status(%d)", __FUNCTION__, __LINE__, status);
		http_error_reply(http_client, 400, "Bad request, invalid request header, read error");
		return (-1);
	}

	if (http_hdr_req_parse(http_client->hdr_req) < 0) {
		acl_msg_error("%s(%d): parse hdr_req error", __FUNCTION__, __LINE__);
		http_error_reply(http_client, 400, "Bad request, invalid request header, parse error");
		return (-1);
	}

	return (http_service(http_client));
}

static int on_close_client(ACL_ASTREAM *client acl_unused, void *context)
{
	HTTP_CLIENT *http_client = (HTTP_CLIENT*) context;

	TRACE();
	http_client_free(http_client);
	return (-1);
}

static void http_service_start(HTTP_CLIENT *http_client)
{
	TRACE();
	acl_aio_ctl(http_client->stream,
		ACL_AIO_CTL_CLOSE_HOOK_ADD, on_close_client, http_client,
		ACL_AIO_CTL_END);
	http_hdr_req_get_async(http_client->hdr_req,
			http_client->stream,
			request_header_ready,
			http_client,
			var_cfg_rw_timeout);
}

void http_service_main(ACL_ASTREAM *stream, void *ctx acl_unused)
{
	HTTP_CLIENT *http_client;

	http_client = http_client_new(stream);
	http_service_start(http_client);
}
