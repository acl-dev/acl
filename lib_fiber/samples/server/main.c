#include "lib_acl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lib_fiber.h"

static void echo_client(ACL_VSTREAM *cstream)
{
	char  buf[8192];
	int   ret;

	while (1) {
		ret = acl_vstream_gets(cstream, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF) {
			printf("gets error\r\n");
			break;
		}
		buf[ret] = 0;
		printf("gets line: %s", buf);

		if (acl_vstream_writen(cstream, buf, ret) == ACL_VSTREAM_EOF) {
			printf("write error\r\n");
			break;
		}
	}

	acl_vstream_close(cstream);
}

static void fiber_accept(void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx;

	for (;;) {
		ACL_VSTREAM *cstream = acl_vstream_accept(sstream, NULL, 0);
		if (cstream == NULL) {
			printf("acl_vstream_accept error %s\r\n",
				acl_last_serror());
			break;
		}

		printf("accept one\r\n");
		echo_client(cstream);
	}

	acl_vstream_close(sstream);
}

int main(void)
{
	const char *addr = "0.0.0.0:8089";
	ACL_VSTREAM *sstream = acl_vstream_listen(addr, 128);

	fiber_init();
	if (sstream == NULL) {
		printf("acl_vstream_listen error %s\r\n", acl_last_serror());
		return 1;
	}

	acl_non_blocking(ACL_VSTREAM_SOCK(sstream), ACL_NON_BLOCKING);

	printf("%s: call fiber_creater\r\n", __FUNCTION__);
	fiber_create(fiber_accept, sstream, 32768);
	printf("call fiber_schedule\r\n");
	fiber_schedule();

	return 0;
}
