#include "stdafx.h"
#include "test_io.h"

#ifdef	__linux__
#include <sys/eventfd.h>

static int read_wait(int fd, int timeo)
{
	struct pollfd pfd;

	printf(">>>%s: fiber-%d, pfd=%p\r\n", __FUNCTION__, acl::fiber::self(), &pfd);
	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = fd;
	pfd.events = POLLIN;

	int n = poll(&pfd, 1, timeo * 1000);
	if (n < 0) {
		printf("poll error: %s\r\n", acl::last_serror());
		return -1;
	}

	if (n == 0) {
		printf("poll read timeout: %s\r\n", acl::last_serror());
		return 0;
	}

	printf("%s: fd=%d is readable!\r\n", __FUNCTION__, fd);
	return 1;
}

static bool fiber_read(int fd, long long& out)
{
	if (read_wait(fd, 2) <= 0) {
		printf("read_wait error for fd=%d\r\n", fd);
		return false;
	}

	long long n;
	ssize_t ret = read(fd, &n, sizeof(n));
	if (ret != sizeof(n)) {
		printf("read from eventfd %d error %s\r\n",
			fd, acl::last_serror());
		return false;
	} else {
		out = n;
		return true;
	}
}

#endif  // __linux__

int test_poll(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
#ifdef	__linux__
	int fd = eventfd(0, 0);

	if (fd == -1) {
		printf("create eventfd error %s\r\n", acl::last_serror());
		return -1;
	}

	long long out = 0, in = 1000000;
	int shared_stack = 0;

	AUT_INT(test_line, "shared_stack", shared_stack, 0);

	if (shared_stack) {
		printf(">>>fiber's stack shared\r\n");
		go_share(8000) [=, &out] {
			(void) fiber_read(fd, out);
		};
	} else {
		printf(">>>fiber's stack no-shared\r\n");
		go[=, &out] {
			(void) fiber_read(fd, out);
		};
	}

	go[=] {
		long long n = in;
		ssize_t ret = write(fd, &n, sizeof(n));
		if (ret != sizeof(n)) {
			printf("write to eventfd %d error %s\r\n",
				fd, acl::last_serror());
		}
	};

	acl::fiber::schedule();

	if (out == in) {
		printf("Ok, the result read from eventfd: %lld\r\n", out);
		return 0;
	} else {
		printf("Err, the result is %lld, but need %lld\r\n", out, in);
		return -1;
	}
#else
	printf("eventfd only be supported on Linux\r\n");
	return 0;
#endif
}
