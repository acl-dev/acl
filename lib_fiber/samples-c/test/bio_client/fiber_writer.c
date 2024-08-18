#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include "lib_acl.h"
#include "fiber/lib_fiber.h"
#include "fiber_client.h"

void fiber_writer(ACL_FIBER *fiber acl_unused, void *ctx)
{
	FIBER_CTX *fc = (FIBER_CTX *) ctx;
	int   ret, nwrite = 0;
	char *buf = malloc(__max_length);

	fc->writer = acl_fiber_running();

	memset(buf, 'X', __max_length);
	buf[__max_length - 1] = '\n';
	buf[__max_length - 2] = '\r';

	while (1) {
		if (__rw_timeout > 0) {
			int tmp = check_write(fc->fd, __rw_timeout);
			if (tmp < 0) {
				printf("check_write error=%s, fd: %d\r\n",
					acl_last_serror(), fc->fd);
				break;
			}
			if (tmp == 0) {
				printf("can't write 0, fd: %d\r\n", fc->fd);
				if (acl_fiber_killed(NULL)) {
					printf("writer be killed\r\n");
					break;
				}
				continue;
			}
		}

		if ((ret = write(fc->fd, buf, __max_length)) < 0) {
			if (errno == EINTR)
				continue;

			printf("write error, fd: %d\r\n", fc->fd);
			break;
		}

		__count++;
		nwrite += ret;
	}

	if (fc->reader != NULL) {
		ACL_FIBER *reader = fc->reader;
		fc->reader = NULL;
		printf("=============kill reader==============\r\n");
		acl_fiber_kill(reader);
		printf("============kill reader ok============\r\n");
	}

	acl_fiber_sem_post(fc->sem_exit);
}
