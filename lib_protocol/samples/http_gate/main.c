#include "lib_acl.h"
#include "lib_protocol.h"
#include <stdio.h>
#include <stdlib.h>

static void init(void)
{
	acl_lib_init();
}

static void end(void)
{
	acl_lib_end();
}

static void thread_run(void *arg)
{
	ACL_VSTREAM *client = (ACL_VSTREAM*) arg;

	acl_vstream_close(client);
}
	
static void run(void)
{
	const char *addr = "0.0.0.0:8089";
	ACL_VSTREAM *sstream = acl_vstream_listen(addr, 128);
	acl_pthread_pool_t *pool;

	if (sstream == NULL) {
		acl_msg_error("listen %s error(%s)", addr, acl_last_serror());
		return;
	}

	printf("listening on %s ...\n", addr);
	pool = acl_thread_pool_create(100, 120);
	while (1) {
		ACL_VSTREAM *client = acl_vstream_accept(sstream, NULL, 0);
		if (client == NULL) {
			acl_msg_error("accept error(%s)", acl_last_serror());
			break;
		}
		acl_pthread_pool_add(pool, thread_run, client);
	}

	acl_vstream_close(sstream);
}

static void test(void)
{
	HTTP_HDR_REQ *hdr_req = http_hdr_req_new();
	HTTP_HDR_ENTRY *entry;
	ACL_VSTRING *buf = acl_vstring_alloc(256);

	entry = http_hdr_entry_new("GET / HTTP/1.1");
	http_hdr_append_entry(&hdr_req->hdr, entry);

	entry = http_hdr_entry_new("HOST: www.test.com");
	http_hdr_append_entry(&hdr_req->hdr, entry);

	entry = http_hdr_entry_new("Cookie: utmcsr=cs.tv.test.com");
	http_hdr_append_entry(&hdr_req->hdr, entry);

	http_hdr_build(&hdr_req->hdr, buf);
	http_hdr_print(&hdr_req->hdr, "---request hdr---");

	http_hdr_entry_replace2(&hdr_req->hdr, "Cookie", "test.com", "test.com.cn", 1);
	http_hdr_build(&hdr_req->hdr, buf);
	http_hdr_print(&hdr_req->hdr, "---request hdr---");

	acl_vstring_free(buf);
	http_hdr_req_free(hdr_req);
	exit (0);
}

int main(int argc acl_unused, char *argv[] acl_unused)
{
	init();
	test();
	run();
	end();
	return (0);
}
