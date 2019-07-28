#ifndef	__HTTP_VHOST_INCLUDE_H__
#define	__HTTP_VHOST_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct HTTP_VPATH {
	ACL_VSTRING *vpath;	/* virtual path from url */
	ACL_VSTRING *path;	/* local path */
#define	HTTP_TYPE_DOC	0
#define	HTTP_TYPE_CGI		1
#define	HTTP_TYPE_FCGI		2
	int   type;
} HTTP_VPATH;

typedef struct HTTP_VHOST {
	char  host[256];
	ACL_HTABLE *vpath_table;
	HTTP_VPATH *vpath_root;
	char  default_page[256];
} HTTP_VHOST;

void http_vhost_init(void);
void http_vhost_destroy(void);
HTTP_VHOST *http_vhost_add_def(const char *host, const char *root_path, const char *default_page);
HTTP_VHOST *http_vhost_add(const char *host, const char *root_path, const char *default_page);
void http_vpath_add(HTTP_VHOST *http_vhost, const char *vpath, const char *path, int type);
const HTTP_VHOST *http_vhost_find(const char *host);
const HTTP_VPATH *http_vpath_find(const HTTP_VHOST *http_vhost, const char *url_path);

#ifdef	__cplusplus
}
#endif

#endif
