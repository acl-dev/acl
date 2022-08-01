#include "lib_acl.h"
#ifdef	ACL_FREEBSD
#include <pthread_np.h>
#endif
#include <stdio.h>

int main(void)
{
	char buf[256];
	long long n = -1;

	acl_snprintf(buf, sizeof(buf), "n=%lld\r\n", (unsigned long) n);
	printf("buf=%s\r\n", buf);

	printf("current acl version: %s\r\n", acl_version());
	printf("ACL_VSTREAM's size: %d\r\n", (int) sizeof(ACL_VSTREAM));
	printf("main tid: %lu\r\n", acl_main_thread_self());
#if defined(__FreeBSD__) && (__FreeBSD__ >= 9)
	printf("tid: %lu\r\n", (unsigned long) pthread_getthreadid_np());
#else
	printf("tid: %lu\r\n", (unsigned long) acl_pthread_self());
#endif
	return (0);
}
