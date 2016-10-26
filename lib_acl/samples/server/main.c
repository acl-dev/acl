#include "lib_acl.h"
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
	char  buf[8192];
	int   ret;


	ret = acl_vstream_read(client, buf, sizeof(buf));
	if (ret == ACL_VSTREAM_EOF) {
		acl_vstream_close(client);
		return;
	}

	buf[ret] = 0;
	printf("(%s)\n", buf);
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

int main(int argc acl_unused, char *argv[] acl_unused)
{
	init();
	run();
	end();
	return (0);
}
