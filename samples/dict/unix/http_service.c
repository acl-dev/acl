#include "lib_acl.h"
#include "lib_protocol.h"
#include "dict_pool.h"
#include "service_main.h"
#include "http_service.h"

#define	STR	acl_vstring_str
#define	LEN	ACL_VSTRING_LEN

static ACL_HTABLE *__db_table = NULL;
static __thread ACL_VSTREAM *__stream_out = NULL;

static const char *__partions[] = {
	"./cache1",
	"./cache2",
	"./cache3",
	"./cache4"
};
static int   __partions_size = 4;

/* 初始化函数 */
void http_service_init(void *init_ctx acl_unused)
{
	const char *myname = "http_service_init";
	DICT_POOL *pool;
	ACL_ARGV *argv;
	ACL_VSTRING *buf = acl_vstring_alloc(256);
	char *ptr, *name;
	int   i, n, has_default_db = 0;

	http_init(NULL);
	dict_pool_init();

	if (__db_table != NULL)
		acl_msg_fatal("%s(%d): service_init been called more than once",
			myname, __LINE__);

	__db_table = acl_htable_create(10, 0);

	argv = acl_argv_split(var_cfg_dbnames, ", ");
	if (argv == NULL)
		acl_msg_fatal("%s(%d): db_names(%s) invalid", myname, __LINE__, var_cfg_dbnames);

	for (i = 0; i < argv->argc; i++) {
		name = argv->argv[i];
		ptr = strchr(name, ':');
		if (ptr == NULL) {
			acl_msg_warn("%s(%d): dbname(%s) use one db",
				myname, __LINE__, name);
			n = 1;
		} else {
			*ptr++ = 0;
			n = atoi(ptr);
		}
		if (n <= 0)
			n = 1;
		acl_lowercase(name);
		/* acl_vstring_sprintf(buf, "btree:%s/%s", var_cfg_dbpath, name); */
		pool = dict_pool_new(__partions, __partions_size, "btree",
			var_cfg_dbpath, name, 1);
		if (acl_htable_enter(__db_table, name, (char*) pool) == NULL)
			acl_msg_fatal("%s(%d): add %s error", myname, __LINE__, name);
		if (strcmp(name, "default") == 0)
			has_default_db = 1;
	}
	acl_argv_free(argv);

	if (!has_default_db) {
		/*
		acl_vstring_sprintf(buf, "btree:%s/default", var_cfg_dbpath);
		pool = dict_pool_new(STR(buf), 8);
		*/
		pool = dict_pool_new(__partions, __partions_size, "btree",
			var_cfg_dbpath, "default", 1);
		if (acl_htable_enter(__db_table, "default", (char*) pool) == NULL)
			acl_msg_fatal("%s(%d): add default error", myname, __LINE__);
	}

	acl_vstring_free(buf);
}

static void dict_pool_free_fn(void *arg)
{
	DICT_POOL *pool = (DICT_POOL*) arg;

	dict_pool_free(pool);
}

void http_service_exit(void *exit_ctx acl_unused)
{
	if (__db_table)
		acl_htable_free(__db_table, dict_pool_free_fn);
}

static int set_to_db(ACL_VSTREAM *client acl_unused, HTTP_HDR_REQ *hdr_req,
	DICT_POOL *dict_pool, const char *key, const char *value, size_t len)
{
	const char reply_200_keep[] = "HTTP/1.1 200 OK\r\n"
		"Accept-Ranges: bytes\r\n"
		"Server: dict_http/1.0.0 (Unix)\r\n"
		"Content-Length: 9\r\n"
		"Connection: keep-alive\r\n"
		"\r\n"
		"200 OK!\r\n";
	const char reply_200_close[] = "HTTP/1.1 200 OK\r\n"
		"Accept-Ranges: bytes\r\n"
		"Server: dict_http/1.0.0 (Unix)\r\n"
		"Content-Length: 9\r\n"
		"Connection: close\r\n"
		"\r\n"
		"200 OK!\r\n";


	dict_pool_set(dict_pool, (char*) key, strlen(key), (char*) value, len);
	if (!hdr_req->hdr.keep_alive) {
		(void) acl_vstream_writen(__stream_out, reply_200_close, sizeof(reply_200_close) - 1);
		return (-1);
	}

	if (acl_vstream_writen(__stream_out, reply_200_keep, sizeof(reply_200_keep) - 1) == ACL_VSTREAM_EOF)
		return (-1);
	return (0);
}

static int get_from_db(ACL_VSTREAM *client acl_unused, const HTTP_HDR_REQ *hdr_req,
	DICT_POOL *dict_pool, const char *key)
{
	static char reply_404_close[] = "HTTP/1.1 404 not found\r\n"
		"Accept-Ranges: bytes\r\n"
		"Server: dict_http/1.0.0 (Unix)\r\n"
		"Content-Length: 15\r\n"
		"Connection: close\r\n"
		"\r\n"
		"404 not found\r\n";
	static char reply_200_keep[] = "HTTP/1.1 200 OK\r\n"
		"Accept-Ranges: bytes\r\n"
		"Server: dict_http/1.0.0 (Unix)\r\n"
		"Content-type: text/html\r\n"
		"Connection: keep-alive\r\n";
	static char reply_200_close[] = "HTTP/1.1 200 OK\r\n"
		"Accept-Ranges: bytes\r\n"
		"Server: dict_http/1.0.0 (Unix)\r\n"
		"Content-type: text/html\r\n"
		"Connection: close\r\n";
	const char *found_second_fmt = "Content-Length: %d\r\n\r\n";
	char  found_second[256];
	char *value;
	size_t  size;
	struct iovec iov[3];

	if (var_cfg_use_bdb) {
		value = dict_pool_get(dict_pool, (char*) key, strlen(key), &size);
		if (value == NULL) {
			(void) acl_vstream_writen(__stream_out, reply_404_close, sizeof(reply_404_close) - 1);
			return (-1);
		}
	} else {
		key = key;
		dict_pool = dict_pool;
		value = acl_mystrdup("test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4test4\r\n");
		size = strlen(value);
	}

	snprintf(found_second, sizeof(found_second), found_second_fmt, size);
	if (hdr_req->hdr.keep_alive) {
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

	if (acl_vstream_writevn(__stream_out, iov, 3) == ACL_VSTREAM_EOF) {
		acl_myfree(value);
		return (-1);
	}

	acl_myfree(value);

	if (!hdr_req->hdr.keep_alive)
		return (-1);
	return (0);
}

static int get_body_from_req(ACL_VSTREAM *client, HTTP_HDR_REQ *hdr_req, ACL_VSTRING *sbuf)
{
	http_off_t n;
	char  buf[4096];
	HTTP_REQ *http_req = NULL;
	http_req = http_req_new(hdr_req);

#define	MAX_LEN	1024 * 8

#undef	RETURN
#define	RETURN(_x_) do {  \
	if (http_req) {  \
		http_req->hdr_req = NULL;  \
		http_req_free(http_req);  \
	}  \
	return (_x_);  \
} while (0)

	while (1) {
		n = http_req_body_get_sync2(http_req, client, buf, sizeof(buf));
		if (n < 0) {
			RETURN (-1);
		} else if (n == 0)
			break;
		if (acl_vstring_memcat(sbuf, buf, (size_t) n) == NULL) {
			RETURN (-1);
		}
		if (LEN(sbuf) > MAX_LEN) {
			RETURN (-1);
		}
	}

	RETURN (0);
}

static int http_service(ACL_VSTREAM *client)
{
	const char *myname = "http_service";
	HTTP_HDR_REQ *hdr_req = NULL;
	ACL_VSTRING *sbuf = NULL;
	char  dbname[256];
	const char *ptr, *action, *method, *key;
	DICT_POOL *dict_pool;
	int   ret;

#undef	RETURN
#define	RETURN(_x_) do {  \
	if (hdr_req)  \
		http_hdr_req_free(hdr_req);  \
	if (sbuf) \
		acl_vstring_free(sbuf);  \
	return (_x_);  \
} while (0)

	hdr_req = http_hdr_req_new();
	ret = http_hdr_req_get_sync(hdr_req, client, var_cfg_rw_timeout);
	if (ret != HTTP_CHAT_OK) {
		acl_debug(100, 2) ("%s(%d): get http req hdr error(%d, %s)",
				myname, __LINE__, ret, strerror(errno));
		RETURN (-1);
	}

	if (http_hdr_req_parse(hdr_req) < 0) {
		acl_msg_error("%s(%d); parse req hdr error", myname, __LINE__);
		http_error_reply(client, 400, "Bad request, invalid request header");
		RETURN (-1);
	}

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
		http_error_reply(client, 404, "Dict not exist");
		RETURN (-1);
	}

	method = http_hdr_req_method(hdr_req);
	if (method == NULL) {
		acl_msg_error("%s(%d): no method", myname, __LINE__);
		http_error_reply(client, 400, "Bad request, no Method");
		RETURN (-1);
	}

	key = http_hdr_req_param(hdr_req, "key");
	if (key == NULL) {
		acl_msg_error("%s(%d): no key(%s)", myname, __LINE__, STR(hdr_req->url_part));
		http_error_reply(client, 400, "Bad request, no key");
		RETURN (-1);
	}

	action = http_hdr_req_param(hdr_req, "action");
	if (action == NULL) {
		acl_msg_error("%s(%d): no action", myname, __LINE__);
		http_error_reply(client, 400, "Bad request, no action");
		RETURN (-1);
	} else if (strcasecmp(action, "set") == 0) {
		if (strcasecmp(method, "GET") == 0) {
			ptr = http_hdr_req_param(hdr_req, "value");
			if (ptr == NULL) {
				acl_msg_error("%s(%d): no value", myname, __LINE__);
				http_error_reply(client, 400, "Bad request, no value");
				RETURN (-1);
			}
			ret = set_to_db(client, hdr_req, dict_pool, key, ptr, strlen(ptr));
			RETURN (ret);
		} else if (strcasecmp(method, "POST") == 0) {
			sbuf = acl_vstring_alloc(MAX_LEN);
			if (get_body_from_req(client, hdr_req, sbuf) < 0) {
				RETURN (-1);
			}
			ret = set_to_db(client, hdr_req, dict_pool, key, STR(sbuf), LEN(sbuf));
			RETURN (ret);
		} else {
			acl_msg_error("%s(%d): method(%s) not support",
				myname, __LINE__, method);
			http_error_reply(client, 501 , "Method not support");
			RETURN (-1);
		}
	} else if (strcasecmp(action, "get") == 0) {
		if (strcasecmp(method, "GET") != 0) {
			acl_msg_error("%s(%d): not GET method(%s) for get data",
				myname, __LINE__, method);
			http_error_reply(client, 400, "Bad request, should be GET request");
			RETURN (-1);
		}
		ret = get_from_db(client, hdr_req, dict_pool, key);
		RETURN (ret);
	} else {
		acl_msg_error("%s(%d): unsupport action(%s)",
			myname, __LINE__, action);
		http_error_reply(client, 400, "Bad request, not support action");
		RETURN (-1);
	}
}

/* 协议处理函数入口 */
int http_service_main(ACL_VSTREAM *client, void *run_ctx acl_unused)
{
	if (isatty(ACL_VSTREAM_SOCK(client)))
		__stream_out = ACL_VSTREAM_OUT;
	else
		__stream_out = client;
	return (http_service(client));
}
