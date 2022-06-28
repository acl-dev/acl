
/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include <errno.h>
#include <stdlib.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_iostuff.h"

#endif

/* acl_write_buf - write buffer or bust */

int acl_write_buf(ACL_SOCKET fd, const char *buf, int len, int timeout)
{
	int     count;

	while (len > 0) {
		if (timeout > 0 && acl_write_wait(fd, timeout) < 0)
			return -1;
		count = acl_socket_write(fd, buf, len, 0, NULL, NULL);
		if (count < 0) {
			if (acl_last_error() == ACL_EAGAIN && timeout > 0)
				continue;
			return -1;
		}
		if (count == 0)
			acl_msg_fatal("write returned 0");
		buf += count;
		len -= count;
	}
	return len;
}
