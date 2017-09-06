#include "stdafx.h"

/* Application-specific. */

#include "master.h"

/* master_prefork -- prefork service proccess and return the number */

static int master_prefork(ACL_MASTER_SERV *serv)
{
	const char *myname = "master_prefork";

	if (serv->prefork_proc <= 0)
		return 0;

	if ((serv->flags & ACL_MASTER_FLAG_RELOADING) != 0) {
		int  n;
		for (n = 0; n < serv->prefork_proc; n++)
			acl_master_spawn(serv);

		if (acl_msg_verbose)
			acl_msg_info("%s: service %s prefork %d processes ok",
				myname, serv->name, n);
		return n;
	}
	
	if (serv->total_proc < serv->prefork_proc) {
		int  nproc = serv->prefork_proc - serv->total_proc;
		int  n = serv->max_proc - serv->total_proc;

		/* xxx: sanity check */
		if (n > 0 && n < nproc)
			nproc = n;

		for (n = 0 ; n < nproc; n++)
			acl_master_spawn(serv);

		if (acl_msg_verbose)
			acl_msg_info("%s: service %s prefork %d processes ok",
				myname, serv->name, n);
		return nproc;
	}

	return 0;
}

/* master_avail_event - create child process to handle connection request */

static void master_avail_event(int type, ACL_EVENT *event,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	ACL_MASTER_SERV *serv = (ACL_MASTER_SERV *) context;
	int     n;

	if (type == 0)  /* XXX Can this happen? */
		acl_msg_panic("master_avail_event: null event");
	else if (ACL_MASTER_THROTTLED(serv)) {  /* XXX interface botch */
		for (n = 0; n < serv->listen_fd_count; n++) {
			acl_event_disable_readwrite(event,
				serv->listen_streams[n]);
		}
	} else if (serv->prefork_proc <= 0 || serv->max_proc <= 0)
		acl_master_spawn(serv);
	else if (master_prefork(serv) == 0 && serv->total_proc < serv->max_proc)
		acl_master_spawn(serv);
}

/* acl_master_avail_listen - make sure that someone monitors the listen socket */

void acl_master_avail_listen(ACL_MASTER_SERV *serv)
{
	const char *myname = "acl_master_avail_listen";

	/*
	 * When no-one else is monitoring the service's listen socket,
	 * start monitoring the socket for connection requests. All
	 * this under the restriction that we have sufficient resources
	 * to service a connection request.
	 */
	if (acl_msg_verbose)
		acl_msg_info("%s: avail %d total %d max %d", myname,
			serv->avail_proc, serv->total_proc, serv->max_proc);

	/* when service is throttled or stopped, don't fork or listen again */

	if (ACL_MASTER_THROTTLED(serv)) {
		acl_msg_warn("service %s been delayed for throttled",
			serv->conf);
	} else if (ACL_MASTER_STOPPING(serv)) {
		acl_msg_warn("service %s been delayed for been stopped",
			serv->conf);
	} else if (ACL_MASTER_KILLED(serv)) {
		acl_msg_warn("service %s been delayed for been killed",
			serv->conf);
	} else
		acl_master_avail_listen_force(serv);
}

void acl_master_avail_listen_force(ACL_MASTER_SERV *serv)
{
	const char *myname = "acl_master_avail_listen_force";
	int   i;

	/* prefork services */
	if (serv->prefork_proc > 0 && master_prefork(serv) > 0)
		return;

	if ((serv->flags & ACL_MASTER_FLAG_RELOADING) == 0) {
		/* check if there're idle proc */
		if (serv->avail_proc > 0)
			return;

		/* at last, check the proc limit */
		if (!ACL_MASTER_LIMIT_OK(serv->max_proc, serv->total_proc))
			return;
	}

	if (acl_msg_verbose)
		acl_msg_info("%s(%d), %s: enable events %s",
			__FILE__, __LINE__, myname, serv->name);

	for (i = 0; i < serv->listen_fd_count; i++) {
		acl_event_enable_read(acl_var_master_global_event,
			serv->listen_streams[i], 0,
			master_avail_event, (void *) serv);
	}
}

/* acl_master_avail_cleanup - cleanup */

void acl_master_avail_cleanup(ACL_MASTER_SERV *serv)
{
	int     n;

	for (n = 0; n < serv->listen_fd_count; n++) {
		/* XXX must be last */
		acl_event_disable_readwrite(acl_var_master_global_event,
				serv->listen_streams[n]);
	}
}

/* acl_master_avail_more - one more available child process */

void acl_master_avail_more(ACL_MASTER_SERV *serv, ACL_MASTER_PROC *proc)
{
	const char *myname = "acl_master_avail_more";
	int   n;

	/*
	 * This child process has become available for servicing connection
	 * requests, so we can stop monitoring the service's listen socket.
	 * The child will do it for us.
	 */
	if (acl_msg_verbose)
		acl_msg_info("%s: pid %d (%s)", myname, proc->pid,
				proc->serv->name);
	if (proc->avail == ACL_MASTER_STAT_AVAIL)
		acl_msg_panic("%s(%d), %s: process already available",
				__FILE__, __LINE__, myname);
	serv->avail_proc++;
	proc->avail = ACL_MASTER_STAT_AVAIL;
	if (acl_msg_verbose)
		acl_msg_info("%s: disable events %s", myname, serv->name);
	for (n = 0; n < serv->listen_fd_count; n++) {
		acl_event_disable_readwrite(acl_var_master_global_event,
				serv->listen_streams[n]);
	}
}

/* acl_master_avail_less - one less available child process */

void acl_master_avail_less(ACL_MASTER_SERV *serv, ACL_MASTER_PROC *proc)
{
	const char *myname = "acl_master_avail_less";

	/*
	 * This child is no longer available for servicing connection
	 * requests. When no child processes are available, start monitoring
	 * the service's listen socket for new connection requests.
	 */
	if (acl_msg_verbose)
		acl_msg_info("%s: pid %d (%s)", myname,
				proc->pid, proc->serv->name);
	if (proc->avail != ACL_MASTER_STAT_AVAIL)
		acl_msg_panic("%s(%d), %s: process not available",
				__FILE__, __LINE__, myname);
	serv->avail_proc--;
	proc->avail = ACL_MASTER_STAT_TAKEN;
	acl_master_avail_listen(serv);
}
