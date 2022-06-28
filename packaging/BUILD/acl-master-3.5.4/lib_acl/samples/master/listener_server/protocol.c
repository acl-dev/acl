#include <stdio.h>
#include <stdlib.h>

#include "lib_util.h"

#include "spool_main.h"
#include "protocol.h"

int protocol(SPOOL *spool, ACL_VSTREAM *cstream)
{
	char  buf[4096];
	int   n, ret;

	spool = spool;

	n = acl_vstream_gets(cstream, buf, sizeof(buf));
	if (n == ACL_VSTREAM_EOF)
		return (-1);

	ret = acl_vstream_writen(cstream, buf, n);
	if (ret != n)
		return (-1);

	return (0);
}
