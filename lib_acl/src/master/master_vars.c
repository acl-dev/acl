/* System library. */
#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#endif

#ifdef ACL_UNIX

#include <errno.h>
#include <string.h>
#include <unistd.h>

/* Utility library. */

#include "stdlib/acl_msg.h"
#include "stdlib/acl_vstream.h"
#include "thread/acl_thread.h"
#include "event/acl_events.h"  /* just for var_ACL_event_fd_busy_ifcheck's use */

/* Application-specific. */
#include "master_proto.h"
#include "master_params.h"

 /*
  * Tunable parameters.
  */
ACL_VSTREAM *ACL_MASTER_STAT_STREAM = NULL;
ACL_VSTREAM *ACL_MASTER_FLOW_READ_STREAM = NULL;
ACL_VSTREAM *ACL_MASTER_FLOW_WRITE_STREAM = NULL;
acl_pthread_pool_t *acl_var_master_thread_pool = NULL;

/* acl_master_vars_init - initialize from global configuration file */

void    acl_master_vars_init(int buf_size, int rw_timeout)
{
	const char *myname = "acl_master_vars_init";

	if (ACL_MASTER_STAT_STREAM != NULL)
		acl_vstream_free(ACL_MASTER_STAT_STREAM);
	if (ACL_MASTER_FLOW_READ_STREAM != NULL)
		acl_vstream_free(ACL_MASTER_FLOW_READ_STREAM);
	if (ACL_MASTER_FLOW_WRITE_STREAM != NULL)
		acl_vstream_free(ACL_MASTER_FLOW_WRITE_STREAM);

	ACL_MASTER_STAT_STREAM = acl_vstream_fdopen(ACL_MASTER_STATUS_FD,
			O_RDWR, buf_size, rw_timeout, ACL_VSTREAM_TYPE_SOCK);
	ACL_MASTER_FLOW_READ_STREAM = acl_vstream_fdopen(ACL_MASTER_FLOW_READ,
			O_RDONLY, buf_size, rw_timeout, ACL_VSTREAM_TYPE_SOCK);
	ACL_MASTER_FLOW_WRITE_STREAM = acl_vstream_fdopen(ACL_MASTER_FLOW_WRITE,
			O_WRONLY, buf_size, rw_timeout, ACL_VSTREAM_TYPE_SOCK);
	if (acl_var_master_thread_pool == NULL)
		acl_var_master_thread_pool = acl_thread_pool_create(100, 60);
	if (acl_var_master_thread_pool == NULL)
		acl_msg_fatal("%s(%d): create thread pool error(%s)",
			myname, __LINE__, strerror(errno));
}

void   acl_master_vars_end(void)
{
	if (ACL_MASTER_STAT_STREAM) {
		acl_vstream_close(ACL_MASTER_STAT_STREAM);
		ACL_MASTER_STAT_STREAM = NULL;
	}
	if (ACL_MASTER_FLOW_READ_STREAM) {
		acl_vstream_close(ACL_MASTER_FLOW_READ_STREAM);
		ACL_MASTER_FLOW_READ_STREAM = NULL;
	}
	if (ACL_MASTER_FLOW_WRITE_STREAM) {
		acl_vstream_close(ACL_MASTER_FLOW_WRITE_STREAM);
		ACL_MASTER_FLOW_WRITE_STREAM = NULL;
	}
	if (acl_var_master_thread_pool) {
		acl_pthread_pool_destroy(acl_var_master_thread_pool);
		acl_var_master_thread_pool = NULL;
	}
}

#endif /* ACL_UNIX */
