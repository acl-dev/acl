#include "stdafx.h"
#include "test_io.h"


int test_eventfd(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
#ifdef	__linux__
	int fd = eventfd(0, 0);

	if (fd == -1) {
		printf("create eventfd error %s\r\n", acl::last_serror());
		return -1;
	}

	go[=] {
		long long n;
		ssize_t ret = read(fd, &n, sizeof(n));
		if (ret != sizeof(n)) {
			printf("read from eventfd %d error %s\r\n",
				fd, acl::last_serror());
			return -1;
		}
	};

	go[=] {
		long long n = 1000000;
		ssize_t ret = write(fd, &n, sizeof(n));
		if (ret != sizeof(n)) {
			printf("write to eventfd %d error %s\r\n",
				fd, acl::last_serror());
			return -1;
		}
	};

	acl::fiber::schedule();
#else
	printf("eventfd only be supported on Linux\r\n");
	return 0;
#endif
}
