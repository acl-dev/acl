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
int __max_length  = 1024000;

int check_read(int fd, int timeout)
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

int check_write(int fd, int timeout)
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

void echo_client(int fd)
{
	FIBER_CTX *fc = (FIBER_CTX *) malloc(sizeof(FIBER_CTX));

	fc->fd = fd;

	acl_ring_init(&fc->link);
	fc->reader = NULL;
	fc->writer = NULL;
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
