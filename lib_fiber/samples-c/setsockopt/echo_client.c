#include "stdafx.h"
#include "echo_client.h"

static void set_timeout(ACL_VSTREAM *cstream, int rw_timeout)
{
#if defined(__APPLE__) || defined(_WIN32) || defined(_WIN64)
	if (setsockopt(ACL_VSTREAM_SOCK(cstream), SOL_SOCKET,
		SO_RCVTIMEO, &rw_timeout, sizeof(rw_timeout)) < 0) {
#else
	struct timeval tm;
	tm.tv_sec  = rw_timeout;
	tm.tv_usec = 0;
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
	int   ret, count = 0, ecnt = 0;

#define	SOCK ACL_VSTREAM_SOCK

	if (rw_timeout > 0) {
		set_timeout(cstream, rw_timeout);
	}

	while (1) {
		time_t begin = time(NULL);
		ret = acl_vstream_read(cstream, buf, sizeof(buf) - 1);
		time_t end = time(NULL);

		if (ret == ACL_VSTREAM_EOF) {
			if (errno == EAGAIN && ++ecnt <= 3) {
				printf("EAGAIN, try again, fd: %d, cost=%ld\r\n",
					SOCK(cstream), end - begin);
				continue;
			}
			printf("read error: %s, fd: %d, count: %d, timeout count=%d\r\n",
				acl_last_serror(), SOCK(cstream), count, ecnt);
			break;
		}
		buf[ret] = 0;
		//printf("gets line: %s", buf);

#if 1
		if (rw_timeout >= 5) {
			rw_timeout = 2;
			set_timeout(cstream, rw_timeout);
			printf(">>reset read timeout to %d\r\n", rw_timeout);
		}
#endif

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
