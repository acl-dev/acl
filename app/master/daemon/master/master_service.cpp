#include "stdafx.h"
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

/* Application-specific. */

#include "master_api.h"
#include "master_params.h"
#include "master.h"

ACL_MASTER_SERV *acl_var_master_head = NULL;
ACL_EVENT *acl_var_master_global_event = NULL;

/* acl_master_service_init - init service after loading the main.cf */

void acl_master_service_init(void)
{
	const char *myname = "acl_master_service_init";

	/* use poll other than select or epoll, because poll can be not
	 * limited to 1024 like select, and also can generate no problem
	 * after fork process that poll has no fd handle, but epoll has.
	 */
	if (acl_var_master_global_event == NULL)
		acl_var_master_global_event = acl_event_new_poll(
			acl_var_master_delay_sec, acl_var_master_delay_usec);
	if (acl_var_master_global_event == NULL)
		acl_msg_fatal("%s(%d), %s: acl_event_new null, serr=%s",
			__FILE__, __LINE__, myname, strerror(errno));
}

/* acl_master_service_start - activate service */

int acl_master_service_start(ACL_MASTER_SERV *serv)
{
	const char *myname = "acl_master_service_start";

	if (serv == NULL)
		acl_msg_fatal("%s(%d): serv null", myname, __LINE__);

	/*
	 * Enable connection requests, wakeup timers, and status updates from
	 * child processes.
	 */
	acl_msg_info("%s: starting service %s ...", myname, serv->name);

	if (acl_master_listen_init(serv) == -1) {
		acl_msg_error("%s: service %s start error", myname, serv->name);
		return -1;
	}

	acl_msg_info("%s: service %s listen init ok ...", myname, serv->name);

	acl_master_status_init(serv);
	acl_msg_info("%s: service %s status init ok ...", myname, serv->name);

	acl_master_avail_listen(serv);
	acl_msg_info("%s: service %s avail listen ok ...", myname, serv->name);

	acl_master_wakeup_init(serv);
	acl_msg_info("%s: service %s wakeup init ok ...", myname, serv->name);

	acl_msg_info("%s: service started!", myname);
	return 0;
}

/* acl_master_service_kill - deactivate service */

void acl_master_service_kill(ACL_MASTER_SERV *serv)
{
	/* set killed flag to avoid prefork process */
	serv->flags |= ACL_MASTER_FLAG_KILLED;

	/*
	 * Undo the things that master_service_start() did.
	 */
	acl_master_wakeup_cleanup(serv);
	acl_master_status_cleanup(serv);

	acl_master_avail_cleanup(serv);

	acl_master_listen_cleanup(serv);
	acl_master_kill_children(serv);  /* XXX calls master_avail_listen */
}

/* acl_master_service_stop - close IPC with children only */
void acl_master_service_stop(ACL_MASTER_SERV *serv)
{
	/* set STOPPING flag to avoid prefork process */
	serv->flags |= ACL_MASTER_FLAG_STOPPING;

	/*
	 * Undo the things that master_service_start() did.
	 */
	acl_master_wakeup_cleanup(serv);
	acl_master_status_cleanup(serv);
	acl_master_avail_cleanup(serv);
	acl_master_listen_cleanup(serv);
}

/* acl_master_restart_service - restart service after configuration reload */

void acl_master_service_restart(ACL_MASTER_SERV *serv)
{
	/* Undo some of the things that master_service_start() did. */
	acl_master_wakeup_cleanup(serv);
	acl_master_status_cleanup(serv);

	/* set ACL_MASTER_FLAG_RELOADING flag */
	serv->flags |= ACL_MASTER_FLAG_RELOADING;

	/* if master_stop_kill was set then kill the children with SIGTERM */
	if ((serv->flags & ACL_MASTER_FLAG_STOP_KILL) != 0) {
		ACL_BINHASH_INFO **list;
		ACL_BINHASH_INFO **info;
		ACL_MASTER_PROC   *proc;

		acl_msg_info("kill service %s with SIGTERM", serv->conf);

		info = list = acl_binhash_list(acl_var_master_child_table);
		for (; *info; info++) {
			proc = (ACL_MASTER_PROC *) info[0]->value;
			if (proc->serv == serv)
				(void) kill(proc->pid, SIGTERM);
		}
		acl_myfree(list);
	}

	/* Now undo the undone. */
	acl_master_status_init(serv);

	/* re-listen again or prefork children */
	acl_master_avail_listen_force(serv);

	/* ACL_MASTER_FLAG_RELOADING will be remove in acl_master_spawn */
	acl_master_wakeup_init(serv);
}
