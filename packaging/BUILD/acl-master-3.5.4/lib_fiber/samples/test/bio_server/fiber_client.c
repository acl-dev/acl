#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include "lib_acl.h"
#include "fiber/lib_fiber.h"
#include "fiber_client.h"

int  __nconnect = 0;
int  __count = 0;
char __listen_ip[64];
int  __listen_port = 9001;
int  __listen_qlen = 64;
int  __rw_timeout = 0;
int  __echo_data  = 1;
int  __stack_size = 320000;

static int check_read(int fd, int timeout)
{
	struct pollfd pfd;
	int n;

	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = fd;
	pfd.events = POLLIN;

	n = poll(&pfd, 1, timeout);
	if (n < 0) {
		printf("poll error: %s\r\n", strerror(errno));
		return -1;
	}

	if (n == 0)
		return 0;
	if (pfd.revents & POLLIN)
		return 1;
	else
		return 0;
}

static int check_write(int fd, int timeout)
{
	struct pollfd pfd;
	int n;

	memset(&pfd, 0, sizeof(struct pollfd));
	pfd.fd = fd;
	pfd.events = POLLOUT;

	n = poll(&pfd, 1, timeout);
	if (n < 0) {
		printf("poll error: %s\r\n", strerror(errno));
		return -1;
	}

	if (n == 0)
		return 0;
	if (pfd.revents & POLLOUT)
		return 1;
	else
		return 0;
}

typedef struct DATA {
	ACL_RING entry;
	void  *dat;
	size_t len;
	int    eof;
} DATA;

typedef struct FIBER_CTX {
	int fd;
	ACL_RING link;
	ACL_FIBER_SEM *sem_data;
	ACL_FIBER_SEM *sem_exit;
	ACL_FIBER *reader;
	ACL_FIBER *writer;
} FIBER_CTX;

static void fiber_reader(ACL_FIBER *fiber acl_unused, void *ctx)
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
				continue;
			}
		}

		ret = read(fc->fd, buf, sizeof(buf));
		if (acl_fiber_killed(NULL)) {
			printf("-------reader be killed------\r\n");
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

		data = (DATA *) malloc(sizeof(DATA));
		data->eof = 0;
		data->len = ret;
		data->dat = malloc(ret);
		memcpy(data->dat, buf, ret);

		acl_ring_prepend(&fc->link, &data->entry);
		acl_fiber_sem_post(fc->sem_data);
	}

	data = (DATA *) malloc(sizeof(DATA));
	data->len = 0;
	data->dat = NULL;
	data->eof = 1;
	acl_ring_prepend(&fc->link, &data->entry);
	acl_fiber_sem_post(fc->sem_data);

	acl_fiber_sem_post(fc->sem_exit);
}

static void fiber_writer(ACL_FIBER *fiber acl_unused, void *ctx)
{
	FIBER_CTX *fc = (FIBER_CTX *) ctx;
	int   ret, nwrite = 0;
	DATA *data;
	ACL_RING *entry;

	fc->writer = acl_fiber_running();

	while (1) {
		int n = acl_fiber_sem_wait(fc->sem_data);
		if (n >= 1) {
			printf("==========sem_wait: ===n=%d================\r\n", n);
		}

		if (acl_fiber_killed(NULL)) {
			printf("-----writer be killed-----\r\n");
			break;
		}

		entry = acl_ring_pop_tail(&fc->link);
		if (entry == NULL) {
			printf(">>>no data<<<\r\n");
			continue;
		}

		data = ACL_RING_TO_APPL(entry, DATA, entry);
		if (data->eof) {
			free(data);
			printf("-----eof fd=%d-----\r\n", fc->fd);
			break;
		}

		if (__rw_timeout > 0) {
			int tmp = check_write(fc->fd, __rw_timeout);
			if (tmp < 0) {
				printf("check_write error=%s, fd: %d\r\n",
					acl_last_serror(), fc->fd);
				free(data->dat);
				free(data);
				break;
			}
			if (tmp == 0) {
				printf("can't write 0, fd: %d\r\n", fc->fd);
				free(data->dat);
				free(data);
				continue;
			}
		}

		if ((ret = write(fc->fd, data->dat, data->len)) < 0) {
			free(data->dat);
			free(data);
			if (errno == EINTR)
				continue;
			printf("write error, fd: %d\r\n", fc->fd);
			break;
		}

		free(data->dat);
		free(data);

		__count++;
		nwrite += ret;
		if (acl_fiber_killed(NULL)) {
			printf("----%d---writer be killed----\r\n", __LINE__);
		}
	}

	if (fc->reader) {
		ACL_FIBER *reader = fc->reader;
		fc->reader = NULL;
		acl_fiber_kill(reader);
	}

	acl_fiber_sem_post(fc->sem_exit);
}

void fiber_client(ACL_FIBER *fiber acl_unused, void *ctx)
{
	int  *cfd = (int *) ctx;
	FIBER_CTX *fc = (FIBER_CTX *) malloc(sizeof(FIBER_CTX));

	fc->fd = *cfd;
	free(cfd);

	acl_ring_init(&fc->link);
	fc->sem_data = acl_fiber_sem_create(0);
	fc->sem_exit = acl_fiber_sem_create(0);

	printf("client fiber-%d: fd: %d\r\n", acl_fiber_self(), fc->fd);

	acl_fiber_create(fiber_reader, fc, __stack_size);
	acl_fiber_create(fiber_writer, fc, __stack_size);

	(void) acl_fiber_sem_wait(fc->sem_exit);
	(void) acl_fiber_sem_wait(fc->sem_exit);

	printf("----------------close %d----------------\r\n", fc->fd);
	close(fc->fd);
	acl_fiber_sem_free(fc->sem_data);
	acl_fiber_sem_free(fc->sem_exit);

	while (1) {
		DATA *data;
		ACL_RING *entry = (ACL_RING *) acl_ring_pop_tail(&fc->link);
		if (entry == NULL)
			break;
		data = ACL_RING_TO_APPL(entry, DATA, entry);
		free(data->dat);
		free(data);
	}

	free(fc);

	if (--__nconnect == 0) {
		printf("\r\n----total read/write: %d----\r\n", __count);
		__count = 0;
	}
}

