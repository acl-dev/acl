#include "stdafx.h"
#include "io.h"

ssize_t pread_pwrite(int from, int to, off_t off, ssize_t size)
{
	char buf[4096];
	ssize_t left = size;

	while (left > 0) {
		size_t len  = sizeof(buf) > (size_t) left ?
			(size_t) left : sizeof(buf);
		ssize_t ret = pread(from, buf, len, off);
		if (ret <= 0) {
			//printf("pread over ret=%zd\r\n", ret);
			break;
		}
		ssize_t n = pwrite(to, buf, ret, off);
		if (n != ret) {
			printf("pwrite error=%s, ret=%zd\r\n", strerror(errno), n);
			break;
		}

		left -= n;
		off += n;
	}

	printf("thread-%lu, fiber-%d: copy over, size=%zd, off=%ld, left=%zd\r\n",
		pthread_self(), acl_fiber_self(), size, off, left);
	return size - left;
}
