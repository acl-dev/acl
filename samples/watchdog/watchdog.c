
/* Utility library. */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "lib_util.h"

static void __watchdog_fn(ACL_WATCHDOG *wp, char *arg)
{
	char  myname[] = "__watchdog_fn";
	char *buf;

	wp = wp;
	buf = (char *) arg;
	printf("%s: buf [%s]\n", myname, buf);
	acl_watchdog_start(wp);
}

int     main(int unused_argc, char **unused_argv)
{
	char      myname[] = "main";
	ACL_WATCHDOG *wp;
	ACL_VSTREAM  *vp;
	char      buf[256] = "test before";

	unused_argc = unused_argc;
	unused_argv = unused_argv;

	acl_msg_verbose = 2;

	printf("buf=%s\n", buf);
	vp = acl_vstream_fdopen(0, 0, 0600, 4096, 0);
	if (vp == NULL)
		acl_msg_fatal("%s(%d)->%s: vstream_fdopen err %s",
				__FILE__, __LINE__, myname,
				strerror(errno));

	wp = acl_watchdog_create(10, __watchdog_fn, (char *) buf);
	acl_watchdog_start(wp);

	while (acl_vstream_gets_nonl(vp, buf, sizeof(buf) - 1) != ACL_VSTREAM_EOF) {
		acl_msg_info(">>> your input:%s", buf);
		if (strcasecmp(buf, "quit") == 0 || strcasecmp(buf, "exit") == 0)
			break;
		acl_watchdog_pat();
	}

	acl_watchdog_destroy(wp);

	exit (0);
}
