#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#endif

#ifdef	ACL_UNIX
#include <stdlib.h>
#include "stdlib/acl_msg.h"
#include "master/acl_server_api.h"
#include "template.h"

static int  var_master_log_opened = 0;

void acl_master_log_open(const char *procname)
{
	const char *myname = "acl_master_log_open";
	char *master_log;

	/* use master's log before chroot */
	master_log = getenv("MASTER_LOG");
	if (master_log == NULL) {
		acl_msg_info("%s(%d): no MASTER_LOG's env value", myname, __LINE__);
	} else {
		acl_msg_open(master_log, procname);
		var_master_log_opened = 1;
		if (acl_msg_verbose)
			acl_msg_info("%s(%d): service: %s, log opened now.",
				myname, __LINE__, procname);
	}
}

void master_log_close()
{
	if (var_master_log_opened) {
		acl_msg_close();
		var_master_log_opened = 0;
	}
}
#endif
