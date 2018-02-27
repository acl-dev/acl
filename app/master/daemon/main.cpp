/* System libraries. */
#include "stdafx.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <syslog.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>

#include "version.h"
#include "master/master_params.h"
#include "master/master.h"
#include "manage/manager.h"

char *var_master_procname;

#define STR	acl_vstring_str
#define LEN	ACL_VSTRING_LEN

static void lock_service(void)
{
	ACL_VSTRING *buf = acl_vstring_alloc(128);
	ACL_VSTRING *tmp = acl_vstring_alloc(128);
	ACL_VSTREAM *fp;

	/* create lock file make sure acl_master can only start once */
	acl_vstring_sprintf(buf, "%s/.master.lock", acl_var_master_queue_dir);
	fp = acl_open_lock(STR(buf), O_RDWR | O_CREAT, 0644, tmp);
	if (fp == NULL) {
		acl_msg_fatal("%s(%d): open lock file %s: %s", __FUNCTION__,
			__LINE__, STR(buf), STR(tmp));
	}

	acl_vstring_sprintf(buf, "%lu\n", (long) time(NULL));
	acl_vstream_writen(fp, STR(buf), LEN(buf));

	acl_close_on_exec(ACL_VSTREAM_FILE(fp), ACL_CLOSE_ON_EXEC);
	acl_vstring_free(buf);
	acl_vstring_free(tmp);
}

static void lock_pidfile(void)
{
	ACL_VSTREAM *fp;
	ACL_VSTRING *buf;

	/* open pid file, lock it and write the master's pid value into it */

	buf = acl_vstring_alloc(10);

	fp = acl_open_lock(acl_var_master_pid_file, O_RDWR | O_CREAT, 0644, buf);
	if (fp == NULL) {
		acl_msg_fatal("%s(%d): open lock file %s: %s", __FUNCTION__,
			__LINE__, acl_var_master_pid_file, STR(buf));
	}
	acl_vstring_sprintf(buf, "%*lu\n", (int) sizeof(unsigned long) * 4,
		(unsigned long) acl_var_master_pid);
	acl_vstream_writen(fp, STR(buf), LEN(buf));
	acl_close_on_exec(ACL_VSTREAM_FILE(fp), ACL_CLOSE_ON_EXEC);

	acl_vstring_free(buf);
}

/* usage - show hint and terminate */

static void usage(const char *me)
{
	printf("usage: %s -c root_dir\r\n"
		" -l log_file\r\n"
		" -v [version]\r\n"
		" -V [verbose]\r\n"
		" -h [help]\r\n"
		" -k (keep_mask)\r\n", me);
}

/* main - main program */

int     main(int argc, char **argv)
{
	//ACL_WATCHDOG *watchdog;
	int ch, fd, n, keep_mask = 0;
	ACL_AIO *aio;
	char *ptr;

	/*
	 * Strip and save the process name for diagnostics etc.
	 */
	var_master_procname = acl_mystrdup(acl_safe_basename(argv[0]));
	acl_var_master_pid = getpid();

	acl_var_master_conf_dir = NULL;
	acl_var_master_log_file = NULL;
	acl_open_limit(0);

	while ((ch = getopt(argc, argv, "Vhvc:l:k")) > 0) {
		switch (ch) {
		case 'V':
			acl_msg_verbose++;
			break;
		case 'v':
			printf("%s %s %s(acl-%s)\r\n", MASTER_NAME,
				MASTER_VERSION, MASTER_DATE, acl_version());
			return 0;
		case 'h':
			usage(argv[0]);
			return 0;
		case 'c':
			acl_var_master_conf_dir = acl_mystrdup(optarg);
			break;
		case 'l':
			acl_var_master_log_file = acl_mystrdup(optarg);
			break;
		case 'k':
			keep_mask = 1;
			break;
		default:
			usage(argv[0]);
			return 0;
			/* NOTREACHED */
		}
	}

	/* Initialize. */
	if (!keep_mask)
		umask(077);  /* never fails! */

	/* Don't die when a process goes away unexpectedly. */
	signal(SIGPIPE, SIG_IGN);

	if (acl_var_master_conf_dir == NULL || acl_var_master_log_file == NULL)
		usage(argv[0]);

	ptr = strrchr(acl_var_master_conf_dir, '/');
	if (ptr != NULL && *(ptr + 1) == '\0')  /* as "/opt/master/conf/" */
		*ptr = '\0';
	
	/*
	 * This program takes no other arguments.
	 */
	if (argc > optind)
		usage(argv[0]);

	/*
	 * When running a child process, don't leak any open files that were
	 * leaked to us by our own (privileged) parent process. Descriptors 0-2
	 * are taken care of after we have initialized error logging.
	 * 
	 * Some systems such as AIX have a huge per-process open file limit.
	 * In those cases, limit the search for potential file descriptor leaks
	 * to just the first couple hundred.
	 * 
	 * The Debian post-installation script passes an open file descriptor
	 * into the master process and waits forever for someone to close it.
	 * Because of this we have to close descriptors > 2, and pray that
	 * doing so does not break things.
	 */

	acl_closefrom(3); /* 0: stdin; 1: stdout; 2: stderr */

	/* Initialize logging and exit handler. */

	/*
	 * don't call acl_close_on_exec that the children can use
	 * master's log in starting status
	 * acl_log_close_onexec(0);
	 */

	/* use 0 as the log's fd */
	close(0);
	acl_msg_open(acl_var_master_log_file, var_master_procname);

	/*
	 * If started from a terminal, get rid of any tty association. This also
	 * means that all errors and warnings must go to the syslog daemon.
	 */
	for (fd = 1; fd < 3; fd++) {
		(void) close(fd);
		if (open("/dev/null", (int) O_RDWR, 0) != fd)
			acl_msg_fatal("open /dev/null: %s", acl_last_serror());
	}

	if (setsid() == -1 && getsid(0) != getpid())
		acl_msg_fatal("unable to set session %s", acl_last_serror());

	/*
	 * Make some room for plumbing with file descriptors. XXX This breaks
	 * when a service listens on many ports. In order to do this right we
	 * must change the master-child interface so that descriptors do not
	 * need to have fixed numbers.
	 * 
	 * In a child we need two descriptors for the flow control pipe, one
	 * for child->master status updates and at least one for listening.
	 */
	for (n = 0; n < 5; n++) {
		fd = dup(1);
		if (acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC) < 0)
			acl_msg_fatal("dup(0): %s", acl_last_serror());
		if (acl_msg_verbose)
			acl_msg_info("dup(0), fd = %d", fd);
	}

	/* load main.cf of acl_master */
	acl_master_main_config();

	/* For the unique master service be started, first to lock file */
	lock_service();

	/* just for prefork service -- zsx, 2012.3.24 */
	acl_master_flow_init();

	/* start all services processes */
	acl_master_start_services();

	/* init master manager module */
	manager::get_instance().init(acl_var_master_global_event,
		acl_var_master_manage_addr, acl_var_master_rw_timeout);

	/*
	 * Finish initialization, last part. We must process configuration
	 * files after processing command-line parameters, so that we get
	 * consistent results when we SIGHUP the server to reload
	 * configuration files.
	 */
	acl_master_sigsetup();

	/* Save pid to local file and lock it */
	lock_pidfile();

	acl_msg_info("daemon started -- version %s, configuration %s",
		MASTER_VERSION, acl_var_master_conf_dir);

	/*
	 * Process events. The event handler will execute the read/write/timer
	 * action routines. Whenever something has happened, see if we received
	 * any signal in the mean time. Although the master process appears to do
	 * multiple things at the same time, it really is all a single thread, so
	 * that there are no concurrency conflicts within the master process.
	 */
	//watchdog = acl_watchdog_create(1000, (ACL_WATCHDOG_FN) 0, (char *) 0);

	for (;;) {
#ifdef HAS_VOLATILE_LOCKS
		if (acl_myflock(ACL_VSTREAM_FILE(lock_fp), ACL_INTERNAL_LOCK,
			ACL_FLOCK_OP_EXCLUSIVE) < 0) {

			acl_msg_fatal("refresh exclusive lock: %m");
		}
#endif

		//acl_watchdog_start(watchdog);  /* same as trigger servers */

		acl_event_loop(acl_var_master_global_event);
		aio = manager::get_instance().get_aio();
		if (aio)
			acl_aio_check(aio);

		if (acl_var_master_gotsighup) {
			acl_msg_info("reload configuration");
			acl_var_master_gotsighup = 0;   /* this first */
			acl_master_refresh();           /* then this */
		}
		if (acl_var_master_gotsigchld) {
			if (acl_msg_verbose)
				acl_msg_info("got sigchld");
			acl_var_master_gotsigchld = 0;  /* this first */
			acl_master_reap_child();        /* then this */
		}
		if (acl_var_master_stopped)
			break;
	}

#define EQ	!strcasecmp

	if (EQ(acl_var_master_stop_kill, "true")
		|| EQ(acl_var_master_stop_kill, "yes")
		|| EQ(acl_var_master_stop_kill, "on")) {

		acl_master_delete_all_children();
	}
	return 0;
}
