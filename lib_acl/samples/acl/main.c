#include "lib_acl.h"
#ifdef	ACL_FREEBSD
#include <pthread_np.h>
#endif
#include <stdio.h>

int main(void)
{
	printf("current acl version: %s\r\n", acl_version());
	printf("ACL_VSTREAM's size: %d\r\n", (int) sizeof(ACL_VSTREAM));
	printf("tid: %lu\r\n", (unsigned long) acl_pthread_self());
	printf("main tid: %lu\r\n", acl_main_thread_self());
#ifdef	ACL_FREEBSD
	printf("tid: %lu\r\n", (unsigned long) pthread_getthreadid_np());
#endif
	return (0);
}
