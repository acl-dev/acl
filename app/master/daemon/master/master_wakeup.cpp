#include "stdafx.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>

/* Global library. */

#include "trigger/trigger.h"

/* Application-specific. */

#include "master_params.h"
#include "master.h"

/* master_wakeup_timer_event - wakeup event handler */

static void master_wakeup_timer_event(int type acl_unused,
	ACL_EVENT *event, void *context)
{
	const char *myname = "master_wakeup_timer_event";
	ACL_MASTER_SERV *serv = (ACL_MASTER_SERV *) context;
	static char wakeup = ACL_TRIGGER_REQ_WAKEUP;
	int     status = 0;

	if (event != acl_var_master_global_event)
		abort();

	/*
	 * Don't wakeup services whose automatic wakeup feature was
	 * turned off in the mean time.
	 */
	if (serv->wakeup_time == 0)
		return;

	/*
	 * Don't wakeup services whose is ACL_MASTER_SERV_TYPE_SOCK
	 */
	if (serv->type == ACL_MASTER_SERV_TYPE_SOCK)
		return;

	/*
	 * Don't wake up services that are throttled. Find out what
	 * transport to use. We can't block here so we choose a short timeout.
	 */
#define BRIEFLY	1

	if (ACL_MASTER_THROTTLED(serv) == 0) {
		if (acl_msg_verbose)
			acl_msg_info("%s: service %s", myname, serv->name);

		switch (serv->type) {
		case ACL_MASTER_SERV_TYPE_INET:
			status = acl_inet_trigger(acl_var_master_global_event,
				serv->name, &wakeup, sizeof(wakeup), BRIEFLY);
			break;
		case ACL_MASTER_SERV_TYPE_UNIX:
			status = ACL_LOCAL_TRIGGER(acl_var_master_global_event,
				serv->name, &wakeup, sizeof(wakeup), BRIEFLY);
			break;

		/*
		 * If someone compromises the postfix account then this must
		 * not overwrite files outside the chroot jail. Countermeasures:
		 * 
		 * - Limit the damage by accessing the FIFO as postfix not root.
		 * 
		 * - Have fifo_trigger() call safe_open() so we won't follow
		 * arbitrary hard/symlinks to files in/outside the chroot jail.
		 * 
		 * - All non-chroot postfix-related files must be root owned
		 *   (or postfix check complains).
		 * 
		 * - The postfix user and group ID must not be shared with
		 *   other applications (says the INSTALL documentation).
		 * 
		 * Result of a discussion with Michael Tokarev, who received
		 * his insights from Solar Designer, who tested Postfix with
		 * a kernel module that is paranoid about open() calls.
		 */
		case ACL_MASTER_SERV_TYPE_FIFO:
			if (acl_var_master_limit_privilege)
				acl_set_eugid(acl_var_master_owner_uid,
					acl_var_master_owner_gid);
			status = acl_fifo_trigger(acl_var_master_global_event,
				serv->name, &wakeup, sizeof(wakeup), BRIEFLY);
			if (acl_var_master_limit_privilege)
				acl_set_ugid(getuid(), getgid());
			break;
		default:
			acl_msg_panic("%s: unknown service type: %d",
				myname, serv->type);
		}
		if (status < 0)
			acl_msg_warn("%s: service %s: %s",
				myname, serv->name, strerror(errno));
	}

	/*
	 * Schedule another wakeup event.
	 */
	acl_event_request_timer(acl_var_master_global_event,
		master_wakeup_timer_event, (void *) serv,
		(acl_int64) serv->wakeup_time * 1000000, 0);
}

/* acl_master_wakeup_init - start automatic service wakeup */

void acl_master_wakeup_init(ACL_MASTER_SERV *serv)
{
	const char *myname = "acl_master_wakeup_init";

	if (serv->wakeup_time == 0 || (serv->flags & ACL_MASTER_FLAG_CONDWAKE))
		return;
	if (acl_msg_verbose)
		acl_msg_info("%s: service %s time %d",
			myname, serv->name, serv->wakeup_time);
	master_wakeup_timer_event(0, acl_var_master_global_event, serv);
}

/* acl_master_wakeup_cleanup - cancel wakeup timer */

void acl_master_wakeup_cleanup(ACL_MASTER_SERV *serv)
{
	const char *myname = "acl_master_wakeup_cleanup";

	/*
	 * Cleanup, even when the wakeup feature has been turned off.
	 * There might still be a pending timer. Don't depend on the code
	 * that reloads the config file to reset the wakeup timer when
	 * things change.
	 */
	if (acl_msg_verbose)
		acl_msg_info("%s: service %s", myname, serv->name);

	acl_event_cancel_timer(acl_var_master_global_event,
		master_wakeup_timer_event, (void *) serv);
}
