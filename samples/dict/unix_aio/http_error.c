#include "lib_acl.h"
#include "http_service.h"

static char reply_error_fmt[] = "HTTP/1.1 %d %s\r\n"
	"Accept-Ranges: bytes\r\n"
	"Server: dict_http/1.0.0 (Unix)\r\n"
	"Content-type: text/html\r\n"
	"Connection: close\r\n\r\n";

static void free_buf(void *arg)
{
	ACL_VSTRING *s = (ACL_VSTRING*) arg;

	acl_vstring_free(s);
}

void http_error_reply(HTTP_CLIENT *http_client, int status, const char *msg)
{
	static __thread ACL_VSTRING *__buf = NULL;
	const ACL_VSTRING *str;
	const char *ptr;
	struct iovec iov[2];

	if (__buf == NULL) {
		__buf = acl_vstring_alloc(256);
		acl_pthread_atexit_add(__buf, free_buf);
	}

	ptr = http_tmpl_title(status);
	str = http_tmpl_get(status);
	
	if (msg == NULL || *msg == 0)
		msg = acl_vstring_str(str);

	acl_vstring_sprintf(__buf, reply_error_fmt, status, ptr);
	
	iov[0].iov_base = acl_vstring_str(__buf);
	iov[0].iov_len  = ACL_VSTRING_LEN(__buf);
	iov[1].iov_base = (char*) msg;
	iov[1].iov_len  = strlen(msg);

	acl_aio_writev(http_client->stream, iov, 2);
}
