#include "stdafx.h"
#include "test_io.h"

#ifdef	__linux__
#include <sys/eventfd.h>
#endif

int test_eventfd(AUT_LINE *test_line acl_unused, void *arg acl_unused)
{
#ifdef	__linux__
	int fd = eventfd(0, 0);

	if (fd == -1) {
		printf("create eventfd error %s\r\n", acl::last_serror());
		return -1;
	}

	long long out = 0, in = 1000000;

	go[=, &out] {
		long long n;
		ssize_t ret = read(fd, &n, sizeof(n));
		if (ret != sizeof(n)) {
			printf("read from eventfd %d error %s\r\n",
				fd, acl::last_serror());
		} else {
			out = n;
		}
	};

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
	} else {
		printf("Err, the result is %lld, but need %lld\r\n", out, in);
	}

	return 0;
#else
	printf("eventfd only be supported on Linux\r\n");
	return 0;
#endif
}
