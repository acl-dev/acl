#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include "lib_acl.h"
#include "fiber/lib_fiber.h"
#include "fiber_client.h"

void fiber_reader(ACL_FIBER *fiber acl_unused, void *ctx)
{
	FIBER_CTX *fc = (FIBER_CTX *) ctx;
	char  buf[81920];
	DATA *data;
	int   ret;
	long long nread = 0;

	fc->reader = acl_fiber_running();

	while (1) {
		if (__rw_timeout > 0) {
			ret = check_read(fc->fd, __rw_timeout);
			if (ret < 0)
				break;
			if (ret == 0) {
				printf("can't read, fd: %d\r\n", fc->fd);
				if (acl_fiber_killed(NULL)) {
					printf("%d-reader be killed-\r\n", __LINE__);
					break;
				}
				continue;
			}
		}

		ret = read(fc->fd, buf, sizeof(buf));

		if (acl_fiber_killed(NULL)) {
			printf("%d-reader be killed-\r\n", __LINE__);
			break;
		}

		if (ret == 0) {
			printf("read close by peer fd: %d, %s\r\n",
				fc->fd, strerror(errno));
			break;
		} else if (ret < 0) {
			if (errno == EINTR) {
				printf("catch a EINTR signal\r\n");
				continue;
			}

			printf("read error %s, fd: %d\n",
				strerror(errno), fc->fd);
			break;
		}

		nread += ret;
		//printf("ret: %d, nread: %lld\r\n", ret, nread);

		if (!__echo_data)
			continue;

		if (0) {
			data = (DATA *) malloc(sizeof(DATA));
			data->eof = 0;
			data->len = ret;
			data->dat = malloc(ret);
			memcpy(data->dat, buf, ret);

			acl_ring_prepend(&fc->link, &data->entry);
			acl_fiber_sem_post(fc->sem_data);
		}
	}

	if (fc->writer) {
		ACL_FIBER *writer = fc->writer;
		fc->writer = NULL;
		acl_fiber_kill(writer);
	}

	acl_fiber_sem_post(fc->sem_exit);
}
