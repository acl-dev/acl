#include "lib_acl.h"
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/resource.h>

int main(void)
{
	struct rlimit rlim, rlim_new;
	char  buf[256];

	acl_chroot_uid(NULL, "zsx");

	printf("curr: %s\r\n", getcwd(buf, sizeof(buf)));

	if (getrlimit(RLIMIT_CORE, &rlim)==0) {
		rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
		if (setrlimit(RLIMIT_CORE, &rlim_new) != 0) {
			/* failed. try raising just to the old max */
			rlim_new.rlim_cur = rlim_new.rlim_max =
				rlim.rlim_max;
			if (setrlimit(RLIMIT_CORE, &rlim_new) != 0)
				printf("set core limit error: %s\n", acl_last_serror());
			else
				printf("set core limit ok\n");
		}
		else
			printf("2 set core limit ok\n");
	} 
	assert(0);
	return (0);
}
