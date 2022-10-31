#include "stdafx.h"
#include "io.h"

ssize_t sendfile_copy(int from, int to, off_t off, ssize_t size)
{
	ssize_t left = size;
	off64_t offset = off;

	while (left > 0) {
		ssize_t ret = sendfile64(to, from, &offset, left);
		if (ret <= 0) {
			printf("sendfile error=%zd, %s\r\n", ret, strerror(errno));
			exit(1);
			break;
		}
		left -= ret;
	}

	return size - left;
}
