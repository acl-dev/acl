#include "stdafx.h"
#include "io.h"

ssize_t splice_copy(int pipefd[2], int from, int to, off_t off, ssize_t len)
{
	unsigned flags = SPLICE_F_MOVE | SPLICE_F_MORE;
	loff_t off_from = off, off_to = off;
	ssize_t left = len;

	while (left > 0) {
		ssize_t ret = splice(from, &off_from, pipefd[1], NULL,
				left, flags);
		if (ret <= 0) {
			printf("splice to pipe error %s\r\n", strerror(errno));
			return -1;
		}
		left -= ret;

		while (ret > 0) {
			ssize_t rr = splice(pipefd[0], NULL, to, &off_to,
					ret, flags);
			if (rr <= 0) {
				printf("splice to file error %s\r\n",
					strerror(errno));
				return -1;
			}
			ret -= rr;
		}
	}

	return len - left;
}
