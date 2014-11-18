/* System libraries. */
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
#include "event/acl_events.h"

/* Application-specific. */

#include "../master_params.h"
#include "master.h"

ACL_MASTER_SERV *acl_var_master_head = NULL;
ACL_EVENT *acl_var_master_global_event = NULL;

/* acl_master_start_service - activate service */

void    acl_master_start_service(ACL_MASTER_SERV *serv)
{
	const char *myname = "acl_master_start_service";

	if (serv == NULL)
		acl_msg_fatal("%s(%d): serv null", myname, __LINE__);

	if (acl_var_master_global_event == NULL)
		acl_var_master_global_event = acl_event_new_select(
			acl_var_master_delay_sec, acl_var_master_delay_usec);
	if (acl_var_master_global_event == NULL)
		acl_msg_fatal("%s(%d)->%s: acl_event_new null, serr=%s",
			__FILE__, __LINE__, myname, strerror(errno));

	/*
	 * Enable connection requests, wakeup timers, and status updates from
	 * child processes.
	 */
	acl_msg_info("%s: starting service %s ...", myname, serv->name);

	acl_master_listen_init(serv);
	acl_msg_info("%s: service %s listen init ok ...", myname, serv->name);
	acl_master_status_init(serv);
	acl_msg_info("%s: service %s status init ok ...", myname, serv->name);
	acl_master_avail_listen(serv);
	acl_msg_info("%s: service %s avail listen ok ...", myname, serv->name);
	acl_master_wakeup_init(serv);
	acl_msg_info("%s: service %s wakeup init ok ...", myname, serv->name);

	acl_msg_info("%s: service started!", myname);
}

/* acl_master_stop_service - deactivate service */

void    acl_master_stop_service(ACL_MASTER_SERV *serv)
{
	/*
	 * Undo the things that master_start_service() did.
	 */
	acl_master_wakeup_cleanup(serv);
	acl_master_status_cleanup(serv);
	acl_master_avail_cleanup(serv);
	acl_master_listen_cleanup(serv);
}

/* acl_master_restart_service - restart service after configuration reload */

void    acl_master_restart_service(ACL_MASTER_SERV *serv)
{
	const char *myname = "acl_master_restart_service";

	if (acl_var_master_global_event == NULL)
		acl_var_master_global_event = acl_event_new_select(
			acl_var_master_delay_sec, acl_var_master_delay_usec);
	if (acl_var_master_global_event == NULL)
		acl_msg_fatal("%s(%d)->%s: acl_event_new null, serr=%s",
			__FILE__, __LINE__, myname, strerror(errno));

	/* Undo some of the things that master_start_service() did. */
	acl_master_wakeup_cleanup(serv);
	acl_master_status_cleanup(serv);

	/* Now undo the undone. */
	acl_master_status_init(serv);

	/* set ACL_MASTER_FLAG_RELOADING flag */
	serv->flags |= ACL_MASTER_FLAG_RELOADING;

	acl_master_avail_listen(serv);

	/* ACL_MASTER_FLAG_RELOADING will be remove in acl_master_spawn */

	acl_master_wakeup_init(serv);
}
#endif /* ACL_UNIX */
