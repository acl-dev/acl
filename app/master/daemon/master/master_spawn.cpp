#include "stdafx.h"

#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

/* Application-specific. */

#include "master_params.h"
#include "master.h"

ACL_BINHASH *acl_var_master_child_table = NULL;
static void master_unthrottle(ACL_MASTER_SERV *serv);

void acl_master_spawn_init(void)
{
	if (acl_var_master_child_table == 0)
		acl_var_master_child_table = acl_binhash_create(0, 0);
}

/* master_unthrottle_wrapper - in case (char *) != (struct *) */

static void master_unthrottle_wrapper(int type acl_unused,
	ACL_EVENT *event acl_unused, void *ptr)
{
	ACL_MASTER_SERV *serv = (ACL_MASTER_SERV *) ptr;

	/*
	 * This routine runs after expiry of the timer set
	 * in master_throttle(), which gets called when it
	 * appears that the world is falling apart.
	 */
	master_unthrottle(serv);
}

/* master_unthrottle - enable process creation */

static void master_unthrottle(ACL_MASTER_SERV *serv)
{
	/*
	 * Enable process creation within this class.
	 * Disable the "unthrottle" timer just in case
	 * we're being called directly from the cleanup
	 * routine, instead of from the event manager.
	 */
	if ((serv->flags & ACL_MASTER_FLAG_THROTTLE) != 0) {
		serv->flags &= ~ACL_MASTER_FLAG_THROTTLE;
		acl_event_cancel_timer(acl_var_master_global_event,
			master_unthrottle_wrapper, (void *) serv);
		if (acl_msg_verbose)
			acl_msg_info("throttle released for command %s",
				serv->path);
		acl_master_avail_listen(serv);	/* XXX interface botch */
	}
}

/* master_throttle - suspend process creation */

static void master_throttle(ACL_MASTER_SERV *serv)
{
	const char *myname = "master_throttle";

	/*
	 * Perhaps the command to be run is defective,
	 * perhaps some configuration is wrong, or
	 * perhaps the system is out of resources.
	 * Disable further process creation attempts for a while.
	 */
	if ((serv->flags & ACL_MASTER_FLAG_THROTTLE) == 0) {
		serv->flags |= ACL_MASTER_FLAG_THROTTLE;
		acl_event_request_timer(acl_var_master_global_event,
			master_unthrottle_wrapper, (void *) serv,
			(acl_int64) serv->throttle_delay * 1000000, 0);
		if (acl_msg_verbose)
			acl_msg_info("%s(%d)->%s: throttling command %s",
				__FILE__, __LINE__, myname, serv->path);
	}
}

/* acl_master_spawn - spawn off new child process if we can */

static unsigned master_generation = 0;
static ACL_VSTRING *env_gen = 0;

static void start_child(ACL_MASTER_SERV *serv)
{
	const char *myname = "start_child";
	ACL_MASTER_NV *nv;
	int i, n;

	/* MASTER_FLOW_READ_STREAM has been inited in master_vars.c */
	if (acl_var_master_flow_pipe[0] <= ACL_MASTER_FLOW_READ)
		acl_msg_fatal("%s: flow pipe read descriptor <= %d",
			myname, ACL_MASTER_FLOW_READ);
	if (dup2(acl_var_master_flow_pipe[0], ACL_MASTER_FLOW_READ) < 0)
		acl_msg_fatal("%s: dup2: %s", myname, strerror(errno));
	if (close(acl_var_master_flow_pipe[0]) < 0)
		acl_msg_fatal("close %d: %s",
			acl_var_master_flow_pipe[0], strerror(errno));

	/* MASTER_FLOW_WRITE_STREAM has been inited in master_vars.c */
	if (acl_var_master_flow_pipe[1] <= ACL_MASTER_FLOW_WRITE)
		acl_msg_fatal("%s: flow pipe read descriptor <= %d",
			myname, ACL_MASTER_FLOW_WRITE);
	if (dup2(acl_var_master_flow_pipe[1], ACL_MASTER_FLOW_WRITE) < 0)
		acl_msg_fatal("%s: dup2: %s", myname, strerror(errno));
	if (close(acl_var_master_flow_pipe[1]) < 0)
		acl_msg_fatal("close %d: %s",
			acl_var_master_flow_pipe[1], strerror(errno));

	/* status channel */
	acl_vstream_close(serv->status_reader);

	/* MASTER_STAT_STREAM has been inited in master_vars.c*/
	if (serv->status_fd[1] <= ACL_MASTER_STATUS_FD)
		acl_msg_fatal("%s: status file descriptor collision", myname);
	if (dup2(serv->status_fd[1], ACL_MASTER_STATUS_FD) < 0)
		acl_msg_fatal("%s: dup2 status_fd: %s", myname, strerror(errno));

	close(serv->status_fd[1]);

	for (n = 0; n < serv->listen_fd_count; n++) {
		if (serv->listen_fds[n] <= ACL_MASTER_LISTEN_FD + n)
			acl_msg_fatal("%s: listen fd collision", myname);
		if (dup2(serv->listen_fds[n], ACL_MASTER_LISTEN_FD + n) < 0)
			acl_msg_fatal("%s: dup2 listen_fd %d: %s",
				myname, serv->listen_fds[n], strerror(errno));
		(void) close(serv->listen_fds[n]);
		acl_vstream_free(serv->listen_streams[n]);
	}

	acl_vstring_sprintf(env_gen, "%s=%o", ACL_MASTER_GEN_NAME,
		master_generation);
	if (putenv(acl_vstring_str(env_gen)) < 0)
		acl_msg_fatal("%s: putenv: %s", myname, strerror(errno));

	n = acl_array_size(serv->children_env);
	for (i = 0; i < n; i++) {
		nv = (ACL_MASTER_NV *)
			acl_array_index(serv->children_env, i);
		if (nv == NULL)
			break;
		setenv(nv->name, nv->value, 1);
	}

	/* begin to call the child process */
	if (acl_msg_verbose)
		acl_msg_info("%s: cmd = %s", myname, serv->path);

	/* help programs written by golang to change runing privilege */
	if (serv->owner && *serv->owner) {
		acl_msg_info("%s: acl_chroot_uid %s", myname, serv->owner);
		acl_chroot_uid(NULL, serv->owner);
	}

	execvp(serv->path, serv->args->argv);
	acl_msg_fatal("%s: exec %s: %s", myname, serv->path, strerror(errno));
}

void acl_master_spawn(ACL_MASTER_SERV *serv)
{
	const char *myname = "acl_master_spawn";
	ACL_MASTER_PROC *proc;
	ACL_MASTER_PID   pid;

	if (env_gen == 0)
		env_gen = acl_vstring_alloc(100);

	/*
	 * Sanity checks. The master_avail module is supposed
	 * to know what it is doing.
	 */

	if (!(serv->flags & ACL_MASTER_FLAG_RELOADING)) {
		if (!ACL_MASTER_LIMIT_OK(serv->max_proc, serv->total_proc))
			acl_msg_warn("%s(%d)->%s: at process limit %d",
				__FILE__, __LINE__, myname, serv->total_proc);

		if (serv->avail_proc > 0 && (serv->prefork_proc <= 0
			|| serv->avail_proc > serv->prefork_proc)) {

			acl_msg_warn("%s(%d)->%s: processes available: %d, "
				"processes prefork: %d", __FILE__, __LINE__,
				myname, serv->avail_proc, serv->prefork_proc);
		}
	}

	/* delete ACL_MASTER_FLAG_RELOADING set in acl_master_restart_service */
	else
	        serv->flags &= ~ACL_MASTER_FLAG_RELOADING;

	if (serv->flags & ACL_MASTER_FLAG_THROTTLE)
		acl_msg_warn("%s(%d)-%s: throttled service: %s",
			__FILE__, __LINE__, myname, serv->path);

	/*
	 * Create a child process and connect parent and
	 * child via the status pipe.
	 */
	master_generation += 1;
	switch (pid = fork()) {

	/*
	 * Error. We're out of some essential resource. Best recourse is to
	 * try again later.
	 */
	case -1:
		acl_msg_warn("%s: fork: %s -- throttling", myname, strerror(errno));
		master_throttle(serv);
		break;

	/*
	 * Child process. Redirect child stdin/stdout to the parent-child
	 * connection and run the requested command. Leave child stderr alone.
	 * Disable exit handlers: they should be executed by the parent only.
	 */
	case 0:  /* child process */
		start_child(serv);

		/* NOTREACHED */
		exit(1);

	/*
	 * Parent. Fill in a process member data structure and set up links
	 * between child and process. Say this process has become available.
	 * If this service has a wakeup timer that is turned on only when the
	 * service is actually used, turn on the wakeup timer.
	 */
	default: /* the parent process */
		if (acl_msg_verbose)
			acl_msg_info("spawn cmd %s pid %d", serv->path, pid);
		proc = (ACL_MASTER_PROC *) acl_mycalloc(1, sizeof(ACL_MASTER_PROC));
		proc->serv      = serv;
		proc->pid       = pid;
		proc->gen       = master_generation;
		proc->avail     = 0;
		proc->start     = (long) time(NULL);
		proc->use_count = 0;

		acl_binhash_enter(acl_var_master_child_table,
			(char *) &pid, sizeof(pid), (char *) proc);
		acl_ring_prepend(&serv->children, &proc->me);
		serv->total_proc++;
		acl_master_avail_more(serv, proc);

		if (serv->flags & ACL_MASTER_FLAG_CONDWAKE) {
			serv->flags &= ~ACL_MASTER_FLAG_CONDWAKE;
			acl_master_wakeup_init(serv);
			if (acl_msg_verbose)
				acl_msg_info("start timer for %s", serv->name);
		}

		break;
	}
}

/* master_delete_child - destroy child process info */

static void master_delete_child(ACL_MASTER_PROC *proc)
{
	const char *myname = "master_delete_child";
	ACL_MASTER_SERV *serv;

	/*
	 * Undo the things that master_spawn did. Stop the process if it still
	 * exists, and remove it from the lookup tables. Update the number of
	 * available processes.
	 */
	serv = proc->serv;
	serv->total_proc--;
	if (proc->avail == ACL_MASTER_STAT_AVAIL) {
		if (acl_msg_verbose)
			acl_msg_info("%s(%d)->%s: call master_avail_less",
				__FILE__, __LINE__, myname);
		acl_master_avail_less(serv, proc);
	} else if (ACL_MASTER_LIMIT_OK(serv->max_proc, serv->total_proc)
		&& serv->avail_proc < 1) {
		if (acl_msg_verbose)
			acl_msg_info("%s(%d)->%s: listen again",
				__FILE__, __LINE__, myname);
		acl_master_avail_listen(serv);
	}

	if (acl_msg_verbose > 2)
		acl_msg_info("%s(%d)->%s: delete process id: %d",
			__FILE__, __LINE__, myname, proc->pid);

	acl_binhash_delete(acl_var_master_child_table, (void *) &proc->pid,
		sizeof(proc->pid), (void (*) (void *)) 0);
	acl_ring_detach(&proc->me);
	acl_myfree(proc);
}

static void service_status_exit(ACL_MASTER_SERV *serv, ACL_MASTER_PID pid,
	ACL_WAIT_STATUS_T status)
{
	acl_msg_warn("%s(%d), %s: process %s pid %d exit status %d",
		__FILE__, __LINE__, __FUNCTION__,
		serv->path, pid, WEXITSTATUS(status));

	if (serv->notify_addr != NULL) {
		char buf[256];

		snprintf(buf, sizeof(buf), "exit status %d",
			WEXITSTATUS(status));
		master_warning(serv->notify_addr, serv->notify_recipients,
			serv->path, serv->conf, serv->version, pid, buf);
	}
}

static void service_status_killed(ACL_MASTER_SERV *serv, ACL_MASTER_PID pid,
	ACL_WAIT_STATUS_T status)
{
	acl_msg_warn("%s(%d), %s: process %s pid %d killed by signal %d",
		__FILE__, __LINE__, __FUNCTION__,
		serv->path, pid, WTERMSIG(status));

	if (serv->notify_addr) {
		char buf[256];

		snprintf(buf, sizeof(buf), "killed by %d", WTERMSIG(status));
		master_warning(serv->notify_addr, serv->notify_recipients,
			serv->path, serv->conf, serv->version, pid, buf);
	}
}

static void service_status_throttle(ACL_MASTER_SERV *serv, ACL_MASTER_PID pid,
	ACL_WAIT_STATUS_T status)
{
	acl_msg_warn("%s(%d), %s: bad command startup, path=%s -- throttling,"
		" signal=%d", __FILE__, __LINE__, __FUNCTION__,
		serv->path, WTERMSIG(status));

	if (serv->notify_addr) {
		char buf[256];

		snprintf(buf, sizeof(buf), "signal=%d, exit status %d",
			WTERMSIG(status), WEXITSTATUS(status));
		master_warning(serv->notify_addr, serv->notify_recipients,
			serv->path, serv->conf, serv->version, pid, buf);
	}
}

/* acl_master_reap_child - reap dead children */

void acl_master_reap_child(void)
{
	const char *myname = "acl_master_reap_child";
	ACL_MASTER_SERV  *serv;
	ACL_MASTER_PROC  *proc;
	ACL_MASTER_PID    pid;
	ACL_WAIT_STATUS_T status;

	/*
	 * Pick up termination status of all dead children.
	 * When a process failed on its first job, assume
	 * we see the symptom of a structural problem 
	 * (configuration problem, system running out of resources)
	 * and back off.
	 */
	while ((pid = waitpid((pid_t) - 1, &status, WNOHANG)) > 0) {
		if (acl_msg_verbose)
			acl_msg_info("%s: pid %d", myname, pid);

		proc = (ACL_MASTER_PROC *) acl_binhash_find(
			acl_var_master_child_table, &pid, sizeof(pid));
		if (proc == NULL) {
			acl_msg_warn("master_reap: unknown pid: %d", pid);
			continue;
		}

		if (ACL_NORMAL_EXIT_STATUS(status)) {
			master_delete_child(proc);
			continue;
		}

		serv = proc->serv;

		if (WIFEXITED(status)) {
			service_status_exit(serv, pid, status);
		}

		if (WIFSIGNALED(status)) {
			service_status_killed(serv, pid, status);
		}

		if (proc->use_count == 0
		    && WTERMSIG(status) != SIGTERM
		    && !ACL_MASTER_THROTTLED(serv)
		    && !ACL_MASTER_STOPPING(serv)
		    && !ACL_MASTER_KILLED(serv)) {

			master_throttle(serv);
			service_status_throttle(serv, pid, status);
		}

		master_delete_child(proc);
	}
}

#define WAITING_CHILD	100000

static void waiting_children(int type acl_unused, ACL_EVENT *event, void* ctx)
{
	ACL_MASTER_SERV *serv = (ACL_MASTER_SERV *) ctx;

	acl_master_reap_child();

	if (ACL_MASTER_CHILDREN_SIZE(serv) > 0) {
		acl_msg_info("wait for service %s, total_proc=%d, %d",
			serv->conf, serv->total_proc,
			ACL_MASTER_CHILDREN_SIZE(serv));
		acl_event_request_timer(event, waiting_children,
			(void *) serv, WAITING_CHILD, 0);
	} else {
		acl_msg_info("%s(%d): free service %s been %s, total proc=%d",
			__FUNCTION__, __LINE__, serv->path,
			ACL_MASTER_STOPPING(serv) ? "stopped" : "killed",
			serv->total_proc);
		acl_master_ent_free(serv);
        }
}

/* acl_master_kill_children - kill and delete all child processes of service */

void acl_master_kill_children(ACL_MASTER_SERV *serv)
{
	ACL_BINHASH_INFO **list;
	ACL_BINHASH_INFO **info;
	ACL_MASTER_PROC   *proc;

	info = list = acl_binhash_list(acl_var_master_child_table);
	for (; *info; info++) {
		proc = (ACL_MASTER_PROC *) info[0]->value;
		if (proc->serv == serv)
			(void) kill(proc->pid, SIGTERM);
	}
	acl_myfree(list);

	if ((serv->flags & ACL_MASTER_FLAG_STOP_WAIT) != 0) {
		while (serv->total_proc > 0) {
			acl_master_reap_child();
			if (serv->total_proc > 0)
				acl_doze(100);
		}

		acl_msg_info("%s(%d): free service %s been %s, total proc=%d",
			__FUNCTION__, __LINE__, serv->path,
			ACL_MASTER_STOPPING(serv) ? "stopped" : "killed",
			serv->total_proc);

		acl_master_ent_free(serv);
		return;
	}

	// try waiting children to exit
	if (serv->total_proc > 0)
		acl_master_reap_child();

	// if there are some other children existing, create a timer to wait
	if (serv->total_proc > 0)
		acl_event_request_timer(acl_var_master_global_event,
			waiting_children, (void *) serv, WAITING_CHILD, 0);
	else {
		acl_msg_info("%s(%d): free service %s been %s, total proc=%d",
			__FUNCTION__, __LINE__, serv->path,
			ACL_MASTER_STOPPING(serv) ? "stopped" : "killed",
			serv->total_proc);
		acl_master_ent_free(serv);
	}
}

void acl_master_delete_all_children(void)
{
	ACL_MASTER_SERV *serv, **servp;

	for (servp = &acl_var_master_head; (serv = *servp) != 0;) {
		*servp = serv->next;
		acl_master_service_kill(serv);
	}
}

void acl_master_signal_children(ACL_MASTER_SERV *serv, int sig, int *nsignaled)
{
	const char *myname = "acl_master_signal_children";
	ACL_RING_ITER    iter;
	ACL_MASTER_PROC *proc;
	int n = 0;

	acl_ring_foreach(iter, &serv->children) {
		proc = acl_ring_to_appl(iter.ptr, ACL_MASTER_PROC, me);
		acl_assert(proc);

		if (kill(proc->pid, sig) < 0)
			acl_msg_warn("%s: kill child %d, path %s error %s",
				myname, proc->pid, serv->path, strerror(errno));
		else
			n++;
	}

	if (nsignaled)
		*nsignaled = n;

	acl_msg_info("%s: service %s, path %s, signal %d, children %d,"
		" signaled %d", myname, serv->name, serv->path,
		sig, acl_ring_size(&serv->children), n);
}

void acl_master_sighup_children(ACL_MASTER_SERV *serv, int *nsignaled)
{
	acl_master_signal_children(serv, SIGHUP, nsignaled);
}
