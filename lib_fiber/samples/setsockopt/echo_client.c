#include "stdafx.h"
#include "echo_client.h"

static void set_timeout(ACL_VSTREAM *cstream, int rw_timeout)
{
	struct timeval tm;
	tm.tv_sec  = rw_timeout;
	tm.tv_usec = 0;

#if defined(__APPLE__) || defined(_WIN32) || defined(_WIN64)
	if (setsockopt(ACL_VSTREAM_SOCK(cstream), SOL_SOCKET,
		SO_RCVTIMEO, &__rw_timeout, sizeof(__rw_timeout)) < 0) {
#else
	if (setsockopt(ACL_VSTREAM_SOCK(cstream), SOL_SOCKET,
		SO_RCVTIMEO, &tm, sizeof(tm)) < 0) {
#endif
		printf("%s: setsockopt error: %s\r\n",
			__FUNCTION__, acl_last_serror());
	}
}

void echo_client(ACL_VSTREAM *cstream, int rw_timeout, int echo_data)
{
	char  buf[8192];
	int   ret, count = 0;

#define	SOCK ACL_VSTREAM_SOCK

	while (1) {
		if (rw_timeout > 0) {
			set_timeout(cstream, rw_timeout);
		}

		ret = acl_vstream_read(cstream, buf, sizeof(buf) - 1);
		if (ret == ACL_VSTREAM_EOF) {
			printf("gets error: %s, fd: %d, count: %d\r\n",
				acl_last_serror(), SOCK(cstream), count);
			break;
		}
		buf[ret] = 0;
		//printf("gets line: %s", buf);

		if (!echo_data) {
			count++;
			continue;
		}

		if (acl_vstream_writen(cstream, buf, ret) == ACL_VSTREAM_EOF) {
			printf("write error, fd: %d\r\n", SOCK(cstream));
			break;
		}

		count++;
		//printf("count=%d\n", count);
	}

	acl_vstream_close(cstream);
}
