#include "stdafx.h"

#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>

/* Application-specific. */

#include "master_params.h"
#include "master.h"

typedef struct SERVER_STATUS {
	int status;
	int ok;
	const char *name;
	const char *info;
} SERVER_STATUS;

static SERVER_STATUS __server_status[] = {
	{ ACL_MASTER_STAT_SIGHUP_OK,	1,	"sighup",	"ok"	},
	{ ACL_MASTER_STAT_SIGHUP_ERR,	0,	"sighup",	"error"	},
	{ ACL_MASTER_STAT_START_OK,	1,	"start",	"ok"	},
	{ ACL_MASTER_STAT_START_ERR,	0,	"start",	"error"	},
	{ -1,				0,	NULL,		NULL	},
};

static void service_status(ACL_MASTER_PROC *proc, int status)
{
	const char *path = proc->serv->path;
	ACL_MASTER_SERV *serv = proc->serv;
	SERVER_STATUS *ss = NULL;

	for (int i = 0; __server_status[i].name != NULL; i++) {
		if (__server_status[i].status == status) {
			ss = &__server_status[i];
			break;
		}
	}

	if (ss) {
		acl_msg_info("%s(%d), service=%s, pid=%d, status=%d, %s %s",
			__FUNCTION__, __LINE__, path, (int) proc->pid,
			status, ss->name, ss->info);

		if (serv->callback)
			serv->callback(proc, status, serv->ctx);
		else
			acl_msg_info("%s(%d): callback null, service=%s",
				__FUNCTION__, __LINE__, path);
	} else
		acl_msg_warn("%s(%d), service=%s, pid=%d, status=%d, %s, %s",
			__FUNCTION__, __LINE__, path, (int) proc->pid, status,
			"unknown", "unknown");
}

/* master_status_event - status read event handler */

static void master_status_event(int type, ACL_EVENT *event acl_unused,
	ACL_VSTREAM *stream acl_unused, void *context)
{
	const char *myname = "master_status_event";
	ACL_MASTER_SERV *serv = (ACL_MASTER_SERV *) context;
	ACL_MASTER_STATUS stat_buf;
	ACL_MASTER_PROC *proc;
	ACL_MASTER_PID pid;
	int     n;

	if (type == 0)  /* XXX Can this happen?  */
		return;

	/*
	 * We always keep the child end of the status pipe open, so an EOF
	 * read condition means that we're seriously confused. We use
	 * non-blocking reads so that we don't get stuck when someone sends
	 * a partial message. Messages are short, so a partial read means
	 * someone wrote less than a whole status message. Hopefully the next
	 * read will be in sync again... We use a global child process status
	 * table because when a child dies only its pid is known - we do not
	 * know what service it came from.
	 */

	if (serv->status_reader->rw_timeout > 0) {
		acl_msg_warn("%s:%d, pipe(%d)'s rw_timeout(%d) > 0",
			__FUNCTION__, __LINE__,
			ACL_VSTREAM_SOCK(serv->status_reader),
			serv->status_reader->rw_timeout);
		serv->status_reader->rw_timeout = 0;
	}

	n = acl_vstream_read(serv->status_reader,
		(char *) &stat_buf, sizeof(stat_buf));

	switch (n) {
	case -1:
		acl_msg_warn("%s(%d)->%s: fd = %d, read: %s",
			__FILE__, __LINE__, myname,
			serv->status_fd[0], strerror(errno));
		return;

	case 0:
		acl_msg_panic("%s(%d)->%s: fd = %d, read EOF status",
			__FILE__, __LINE__, myname, serv->status_fd[0]);
		/* NOTREACHED */
		return;

	default:
		acl_msg_warn("%s(%d)->%s: service %s: child (pid %d) "
			"sent partial, fd = %d, status update (%d bytes)", 
			__FILE__, __LINE__, myname, serv->name, stat_buf.pid,
			serv->status_fd[0], n);
		return;

	case sizeof(stat_buf):
		pid = stat_buf.pid;
		if (acl_msg_verbose)
			acl_msg_info("%s: pid = %d, gen = %u, status = %d, "
				"fd = %d", myname, stat_buf.pid, stat_buf.gen,
				stat_buf.status, serv->status_fd[0]);
	} /* end switch */

	/*
	 * Sanity checks. Do not freak out when the child sends garbage because
	 * it is confused or for other reasons. However, be sure to freak out
	 * when our own data structures are inconsistent. A process not found
	 * condition can happen when we reap a process before receiving its
	 * status update, so this is not an error.
	 */

	if (acl_var_master_child_table == 0)
		acl_msg_fatal("%s(%d): acl_var_master_child_table null",
			myname, __LINE__);

	if ((proc = (ACL_MASTER_PROC *) acl_binhash_find(
		acl_var_master_child_table, (char *) &pid, sizeof(pid))) == 0)
	{
		acl_msg_warn("%s(%d)->%s: process id not found: pid = %d,"
			 " status = %d, gen = %u", __FILE__, __LINE__,
			 myname, stat_buf.pid, stat_buf.status, stat_buf.gen);
		return;
	}
	if (proc->gen != stat_buf.gen) {
		acl_msg_warn("%s(%d)->%s: ignoring status update from child "
			"pid %d generation %u", __FILE__, __LINE__,
			myname, pid, stat_buf.gen);
		return;
	}
	if (proc->serv != serv)
		acl_msg_panic("%s(%d)->%s: pointer corruption: %p != %p",
			__FILE__, __LINE__, myname, (void *) proc->serv,
			(void *) serv);

	/*
	 * Update our idea of the child process status. Allow redundant status
	 * updates, because different types of events may be processed out of
	 * order. Otherwise, warn about weird status updates but do not take
	 * action. It's all gossip after all.
	 */
	if (proc->avail == stat_buf.status)
		return;

	switch (stat_buf.status) {
	case ACL_MASTER_STAT_AVAIL:
		proc->use_count++;
		acl_master_avail_more(serv, proc);
		break;
	case ACL_MASTER_STAT_TAKEN:
		acl_master_avail_less(serv, proc);
		break;
	default:
		service_status(proc, stat_buf.status);
		break;
	}
}

/* acl_master_status_init - start status event processing for this service */

void acl_master_status_init(ACL_MASTER_SERV *serv)
{
	const char *myname = "acl_master_status_init";

	/*
	 * Sanity checks.
	 */
	if (serv->status_fd[0] >= 0 || serv->status_fd[1] >= 0)
		acl_msg_panic("%s: status events already enabled", myname);
	if (acl_msg_verbose)
		acl_msg_info("%s: %s", myname, serv->name);

	/*
	 * Make the read end of this service's status pipe non-blocking so that
	 * we can detect partial writes on the child side. We use a duplex pipe
	 * so that the child side becomes readable when the master goes away.
	 */
	if (acl_duplex_pipe(serv->status_fd) < 0)
		acl_msg_fatal("pipe: %s", strerror(errno));

	acl_non_blocking(serv->status_fd[0], ACL_BLOCKING);
	acl_close_on_exec(serv->status_fd[0], ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(serv->status_fd[1], ACL_CLOSE_ON_EXEC);

	/* Must set io rw_timeout to 0 to avoiding blocking read which
	 * will blocking the main event loop.
	 */
	serv->status_reader = acl_vstream_fdopen(serv->status_fd[0],
		O_RDWR, acl_var_master_buf_size, 0, ACL_VSTREAM_TYPE_SOCK);

	if (acl_msg_verbose)
		acl_msg_info("%s(%d)->%s: call acl_event_enable_read, "
			"status_fd = %d", __FILE__, __LINE__,
			myname, serv->status_fd[0]);

	acl_event_enable_read(acl_var_master_global_event,
		serv->status_reader, 0, master_status_event, serv);
}

/* acl_master_status_cleanup - stop status event processing for this service */

void acl_master_status_cleanup(ACL_MASTER_SERV *serv)
{
	const char *myname = "acl_master_status_cleanup";

	/*
	 * Sanity checks.
	 */
	if (serv->status_fd[0] < 0 || serv->status_fd[1] < 0)
		acl_msg_panic("%s: status events not enabled", myname);
	if (acl_msg_verbose)
		acl_msg_info("%s: %s", myname, serv->name);

	/*
	 * Dispose of this service's status pipe after disabling read events.
	 */

	acl_event_disable_readwrite(acl_var_master_global_event,
		serv->status_reader);

	if (close(serv->status_fd[0]) != 0)
		acl_msg_warn("%s: close status descriptor (read side): %s",
			myname, strerror(errno));
	if (close(serv->status_fd[1]) != 0)
		acl_msg_warn("%s: close status descriptor (write side): %s",
			myname, strerror(errno));
	serv->status_fd[0] = serv->status_fd[1] = -1;
	if (serv->status_reader)
		acl_vstream_free(serv->status_reader);
	serv->status_reader = NULL;
}
