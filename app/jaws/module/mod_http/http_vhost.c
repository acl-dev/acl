#include "lib_acl.h"
#include "lib_protocol.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "http_vhost.h"

static ACL_HTABLE *__vhost_table = NULL;
static HTTP_VHOST *__vhost_default = NULL;

static HTTP_VPATH *vpath_new(const char *vpath, const char *path, int type)
{
	const char *myname = "vpath_new";
	HTTP_VPATH *http_vpath;
	char  ebuf[256];

	http_vpath = acl_mycalloc(1, sizeof(HTTP_VPATH));
	if (http_vpath == NULL)
		acl_msg_fatal("%s, %s(%d): calloc error(%s)",
				__FILE__, myname, __LINE__,
				acl_last_strerror(ebuf, sizeof(ebuf)));
	http_vpath->vpath = acl_vstring_alloc(256);
	if (http_vpath->vpath == NULL)
		acl_msg_fatal("%s, %s(%d): calloc error(%s)",
				__FILE__, myname, __LINE__,
				acl_last_strerror(ebuf, sizeof(ebuf)));
	http_vpath->path = acl_vstring_alloc(256);
	if (http_vpath->path == NULL)
		acl_msg_fatal("%s, %s(%d): calloc error(%s)",
				__FILE__, myname, __LINE__,
				acl_last_strerror(ebuf, sizeof(ebuf)));

	/* make sure path format: "/path/.../" */

#define	STR_CP(_vstr, _str) do { \
	if (*_str != '/') \
		acl_vstring_strcpy(_vstr, "/"); \
	acl_vstring_strcat(_vstr, _str); \
	if (acl_vstring_charat(_vstr, ACL_VSTRING_LEN(_vstr) - 1) != '/') \
		acl_vstring_strcat(_vstr, "/"); \
	acl_lowercase(acl_vstring_str(_vstr)); \
} while (0)

	STR_CP(http_vpath->vpath, vpath);
	STR_CP(http_vpath->path, path);
	http_vpath->type = type;

	return (http_vpath);
}

static void vpath_free(HTTP_VPATH *http_vpath)
{
	if (http_vpath == NULL)
		return;

	if (http_vpath->vpath)
		acl_myfree(http_vpath->vpath);
	if (http_vpath->path)
		acl_myfree(http_vpath->path);
	acl_myfree(http_vpath);
}

static HTTP_VHOST *vhost_new(const char *host, HTTP_VPATH *http_vpath_root, const char *default_page)
{
	const char *myname = "vhost_new";
	HTTP_VHOST *http_vhost;
	char  ebuf[256];

	http_vhost = acl_mycalloc(1, sizeof(HTTP_VHOST));
	if (http_vhost == NULL)
		acl_msg_fatal("%s, %s(%d): calloc error(%s)",
				__FILE__, myname, __LINE__,
				acl_last_strerror(ebuf, sizeof(ebuf)));

	ACL_SAFE_STRNCPY(http_vhost->host, host, sizeof(http_vhost->host));
	acl_lowercase(http_vhost->host);
	http_vhost->vpath_root = http_vpath_root;
	if (default_page && *default_page)
		ACL_SAFE_STRNCPY(http_vhost->default_page, default_page, sizeof(http_vhost->default_page));
	else
		ACL_SAFE_STRNCPY(http_vhost->default_page, "index.html", sizeof(http_vhost->default_page));

	http_vhost->vpath_table = acl_htable_create(10, 0);
	if (http_vhost->vpath_table == NULL)
		acl_msg_fatal("%s, %s(%d): acl_htable_create error(%s)",
				__FILE__, myname, __LINE__,
				acl_last_strerror(ebuf, sizeof(ebuf)));
	return (http_vhost);
}

static void free_vpath_fn(void *arg)
{
	HTTP_VPATH *http_vpath = (HTTP_VPATH *) arg;

	vpath_free(http_vpath);
}

static void vhost_free(HTTP_VHOST *http_vhost)
{
	acl_htable_free(http_vhost->vpath_table, free_vpath_fn);
	vpath_free(http_vhost->vpath_root);
	acl_myfree(http_vhost);
}

void http_vhost_init(void)
{
	const char *myname = "http_vhost_init";
	char  ebuf[256];

	__vhost_table = acl_htable_create(10, 0);
	if (__vhost_table == NULL)
		acl_msg_fatal("%s(%d): acl_htable_create error(%s)",
			myname, __LINE__,
			acl_last_strerror(ebuf, sizeof(ebuf)));
}

static void free_vhost_fn(void *arg)
{
	HTTP_VHOST *http_vhost = (HTTP_VHOST *) arg;

	vhost_free(http_vhost);
}

void http_vhost_destroy(void)
{
	if (__vhost_table == NULL)
		return;

	acl_htable_free(__vhost_table, free_vhost_fn);
	__vhost_table = NULL;
}

HTTP_VHOST *http_vhost_add_def(const char *host, const char *root_path, const char *default_page)
{
	HTTP_VPATH *http_vpath;

	if (__vhost_default)
		return (__vhost_default);

	http_vpath = vpath_new("/", root_path, HTTP_TYPE_DOC);

	__vhost_default = vhost_new(host, http_vpath, default_page);

	return (__vhost_default);
}

HTTP_VHOST *http_vhost_add(const char *host, const char *root_path, const char *default_page)
{
	const char *myname = "http_vhost_add";
	HTTP_VHOST *http_vhost;
	HTTP_VPATH *http_vpath;
	char  ebuf[256];

	if (__vhost_default && strcasecmp(__vhost_default->host, host) == 0)
		return (__vhost_default);

	http_vhost = (HTTP_VHOST *) acl_htable_find(__vhost_table, host);

	if (http_vhost != NULL)
		return (http_vhost);

	http_vpath = vpath_new("/", root_path, HTTP_TYPE_DOC);
	http_vhost = vhost_new(host, http_vpath, default_page);

	if (acl_htable_enter(__vhost_table, http_vhost->host, (void *) http_vhost) == NULL)
		acl_msg_fatal("%s(%d): acl_htable_enter error(%s)",
			myname, __LINE__,
			acl_last_strerror(ebuf, sizeof(ebuf)));

	return (http_vhost);
}

void http_vpath_add(HTTP_VHOST *http_vhost, const char *vpath, const char *path, int type)
{
	const char *myname = "http_vpath_add";
	HTTP_VPATH *http_vpath;
	char  ebuf[256];

	http_vpath = (HTTP_VPATH *) acl_htable_find(http_vhost->vpath_table, vpath);
	if (http_vpath)
		return;

	http_vpath = vpath_new(vpath, path, type);

	if (acl_htable_enter(http_vhost->vpath_table,
				acl_vstring_str(http_vpath->vpath),
				(void *) http_vpath) == NULL)
		acl_msg_fatal(" %s(%d): acl_htable_enter error(%s)",
			myname, __LINE__,
			acl_last_strerror(ebuf, sizeof(ebuf)));
}

const HTTP_VHOST *http_vhost_find(const char *host)
{
	HTTP_VHOST *http_vhost;
	char  buf[1024], *ptr;

	ACL_SAFE_STRNCPY(buf, host, sizeof(buf));
	acl_lowercase(buf);

	ptr = strchr(buf, ':');
	if (ptr)
		*ptr = 0;

	http_vhost = (HTTP_VHOST *) acl_htable_find(__vhost_table, buf);
	if (http_vhost == NULL)
		return (__vhost_default);

	return (http_vhost);
}

const HTTP_VPATH *http_vpath_find(const HTTP_VHOST *http_vhost, const char *url_path)
{
	const char *myname = "http_vpath_find";
	HTTP_VPATH *http_vpath;
	ACL_VSTRING *buf = NULL;
	char *ptr, *ptr1;
	char  ebuf[256];

#define	RETURN(_x_) do { \
	if (buf) \
		acl_vstring_free(buf); \
	return (_x_); \
} while (0)

	if (http_vhost == NULL || url_path == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);

	if (*url_path != '/')
		RETURN (http_vhost->vpath_root);

	buf = acl_vstring_alloc(512);
	if (buf == NULL)
		acl_msg_fatal("%s, %s(%d): acl_vstring_alloc error(%s)",
				__FILE__, myname, __LINE__,
				acl_last_strerror(ebuf, sizeof(ebuf)));

	acl_vstring_strcpy(buf, url_path);
	acl_lowercase(acl_vstring_str(buf));

	ptr = acl_vstring_str(buf);
	ptr++;
	ptr1 = strchr(ptr, '/');
	if (ptr1)
		*(++ptr1) = 0;
	else
		RETURN (http_vhost->vpath_root);

	http_vpath = (HTTP_VPATH *) acl_htable_find(http_vhost->vpath_table, acl_vstring_str(buf));

	if (http_vpath == NULL)
		RETURN (http_vhost->vpath_root);

	RETURN (http_vpath);
}
