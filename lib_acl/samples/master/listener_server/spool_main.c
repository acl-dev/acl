#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "lib_util.h"

#include "global.h"
#include "protocol.h"
#include "spool_main.h"

/*----------------------------------------------------------------------------*/
SPOOL *spool_create(int max_threads, int idle_timeout)
{
	char  myname[] = "spool_create";
	SPOOL *spool;

	spool = (SPOOL *) acl_mycalloc(1, sizeof(SPOOL));
	if (spool == NULL)
		acl_msg_fatal("%s(%d): calloc error(%s)",
				myname, __LINE__, strerror(errno));

	spool->h_spool = acl_spool_create(max_threads, idle_timeout);

	return (spool);
}
/*----------------------------------------------------------------------------*/
int spool_start(const SPOOL *spool)
{
	return (acl_spool_start(spool->h_spool));
}
/*----------------------------------------------------------------------------*/
static void __read_notify_callback(int event_type,
				ACL_SPOOL *h_spool,
				ACL_VSTREAM *cstream,
				void *context)
{
	char  myname[] = "__read_notify_callback";
	SPOOL *spool;

	spool = (SPOOL *) context;

	switch (event_type) {
	case ACL_EVENT_READ:
		if (protocol(spool, cstream) < 0) {
			acl_vstream_close(cstream);
		} else {
			acl_spool_enable_read(h_spool,
						cstream,
						var_cfg_client_idle_limit,
						__read_notify_callback,
						(void *) spool);

		}
		break;
	case ACL_EVENT_RW_TIMEOUT:
	case ACL_EVENT_XCPT:
		acl_vstream_close(cstream);
		break;
	default:
		acl_msg_fatal("%s, %s(%d): unknown event type(%d)",
				__FILE__, myname, __LINE__, event_type);
		/* not reached */
		break;
	}
}

void spool_add_worker(SPOOL *spool, ACL_VSTREAM *cstream)
{
	char  myname[] = "spool_add_worker";

	if (cstream == NULL || spool == NULL)
		acl_msg_fatal("%s(%d): input invalid", myname, __LINE__);
	
	/* 将客户端数据流的状态置入事件监控集合中 */
	acl_spool_enable_read(spool->h_spool,
				cstream,
				var_cfg_client_idle_limit,
				__read_notify_callback,
				(void *) spool);
}
/*----------------------------------------------------------------------------*/

