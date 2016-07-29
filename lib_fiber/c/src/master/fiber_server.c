#include "stdafx.h"
#include <stdarg.h>
#include <poll.h>

/* including the internal headers from lib_acl/src/master */
#include "master_proto.h"
#include "master_params.h"
#include "template/master_log.h"

#include "fiber/lib_fiber.h"
#include "fiber.h"

#define STACK_SIZE	64000

int   acl_var_fiber_pid;
char *acl_var_fiber_procname = NULL;
char *acl_var_fiber_log_file = NULL;

int   acl_var_fiber_stack_size = STACK_SIZE;
int   acl_var_fiber_buf_size;
int   acl_var_fiber_rw_timeout;
int   acl_var_fiber_max_debug;
int   acl_var_fiber_enable_core;
int   acl_var_fiber_use_limit;
int   acl_var_acl_fiber_idle_limit;
static ACL_CONFIG_INT_TABLE __conf_int_tab[] = {
	{ "fiber_stack_size", STACK_SIZE, &acl_var_fiber_stack_size, 0, 0 },
	{ "fiber_buf_size", 8192, &acl_var_fiber_buf_size, 0, 0 },
	{ "fiber_rw_timeout", 120, &acl_var_fiber_rw_timeout, 0, 0 },
	{ "fiber_max_debug", 1000, &acl_var_fiber_max_debug, 0, 0 },
	{ "fiber_enable_core", 1, &acl_var_fiber_enable_core, 0, 0 },
	{ "fiber_use_limit", 0, &acl_var_fiber_use_limit, 0, 0 },
	{ "acl_fiber_idle_limit", 0, &acl_var_acl_fiber_idle_limit, 0 , 0 },

	{ 0, 0, 0, 0, 0 },
};

char *acl_var_fiber_queue_dir;
char *acl_var_fiber_log_debug;
char *acl_var_fiber_deny_banner;
char *acl_var_fiber_access_allow;
char *acl_var_fiber_owner;
char *acl_var_fiber_dispatch_addr;
char *acl_var_fiber_dispatch_type;
static ACL_CONFIG_STR_TABLE __conf_str_tab[] = {
	{ "fiber_queue_dir", "", &acl_var_fiber_queue_dir },
	{ "fiber_log_debug", "all:1", &acl_var_fiber_log_debug },
	{ "fiber_deny_banner", "Denied!\r\n", &acl_var_fiber_deny_banner },
	{ "fiber_access_allow", "all", &acl_var_fiber_access_allow },
	{ "fiber_owner", "", &acl_var_fiber_owner },
	{ "fiber_dispatch_addr", "", &acl_var_fiber_dispatch_addr },
	{ "fiber_dispatch_type", "default", &acl_var_fiber_dispatch_type },

	{ 0, 0, 0 },
};

static int    __argc;
static char **__argv;
static int    __daemon_mode = 0;
static void (*__service)(ACL_VSTREAM*, void*) = NULL;
static int   *__service_ctx = NULL;
static char   __service_name[256];
static void (*__service_onexit) = NULL;
static char  *__deny_info = NULL;

static unsigned      __server_generation;
static ACL_VSTREAM **__sstreams;

static int           __server_stopping = 0;
static int           __nclients = 0;
static unsigned      __nused = 0;

static void server_exit(ACL_FIBER *fiber, int status)
{
	acl_msg_info("%s(%d), %s: fiber = %d, service exit now!",
		__FILE__, __LINE__, __FUNCTION__, acl_fiber_id(fiber));
	exit (status);
}

static void server_abort(ACL_FIBER *fiber)
{
	acl_msg_info("%s(%d), %s: service abort now",
		__FILE__, __LINE__, __FUNCTION__);
	server_exit(fiber, 1);
}

static void server_stop(ACL_FIBER *fiber)
{
	if (__server_stopping)
		return;

	__server_stopping = 1;

	acl_fiber_sleep(2);
	server_exit(fiber, 0);
}

static void fiber_monitor_master(ACL_FIBER *fiber, void *ctx)
{
	ACL_VSTREAM *stat_stream = (ACL_VSTREAM *) ctx;
	char  buf[8192];
	int   ret;

	stat_stream->rw_timeout = 0;
	ret = acl_vstream_read(stat_stream, buf, sizeof(buf));
	acl_msg_info("%s(%d), %s: disconnect(%d) from acl_master",
		__FILE__, __LINE__, __FUNCTION__, ret);

	server_exit(fiber, 0);
}

static void fiber_monitor_used(ACL_FIBER *fiber, void *ctx acl_unused)
{
	if (acl_var_fiber_use_limit <= 0) {
		acl_msg_warn("%s(%d), %s: invalid fiber_use_limit(%d)",
			__FILE__, __LINE__, __FUNCTION__,
			acl_var_fiber_use_limit);
		return;
	}

	while (!__server_stopping) {
		if (__nclients > 0) {
			acl_fiber_sleep(1);
			continue;
		}

		if (__nused >= (unsigned) acl_var_fiber_use_limit) {
			acl_msg_info("%s(%d), %s: use_limit reached %d",
				__FILE__, __LINE__, __FUNCTION__,
				acl_var_fiber_use_limit);
			server_stop(fiber);
			break;
		}

		acl_fiber_sleep(1);
	}
}

static void fiber_monitor_idle(ACL_FIBER *fiber, void *ctx acl_unused)
{
	time_t last = time(NULL);

	while (!__server_stopping) {
		if (__nclients > 0) {
			acl_fiber_sleep(1);
			time(&last);
			continue;
		}

		if (time(NULL) - last >= acl_var_acl_fiber_idle_limit) {
			acl_msg_info("%s(%d), %s: idle_limit reached %d",
				__FILE__, __LINE__, __FUNCTION__,
				acl_var_acl_fiber_idle_limit);
			server_stop(fiber);
			break;
		}

		acl_fiber_sleep(1);
	}
}

static void fiber_client(ACL_FIBER *fiber acl_unused, void *ctx)
{
	ACL_VSTREAM *cstream = (ACL_VSTREAM *) ctx;
	const char *peer = ACL_VSTREAM_PEER(cstream);
	char  addr[256];

	if (peer) {
		char *ptr;
		ACL_SAFE_STRNCPY(addr, peer, sizeof(addr));
		ptr = strchr(addr, ':');
		if (ptr)
			*ptr = 0;
	} else
		addr[0] = 0;

	if (addr[0] != 0 && !acl_access_permit(addr)) {
		if (__deny_info && *__deny_info)
			acl_vstream_fprintf(cstream, "%s\r\n", __deny_info);
		acl_vstream_close(cstream);
		return;
	}

	__nclients++;
	__nused++;

	__service(cstream, ctx);

	__nclients--;

	acl_vstream_close(cstream);
}

static int dispatch_receive(int dispatch_fd)
{
	char  buf[256], remote[256], local[256];
	int  fd = -1, ret;
	ACL_VSTREAM *cstream;

	ret = acl_read_fd(dispatch_fd, buf, sizeof(buf) - 1, &fd);
	if (ret < 0 || fd < 0) {
		acl_msg_warn("%s(%d), %s: read from master_dispatch(%s) error",
			__FILE__, __LINE__, __FUNCTION__,
			acl_var_fiber_dispatch_addr);
		return -1;
	}

	buf[ret] = 0;

	cstream = acl_vstream_fdopen(fd, O_RDWR, acl_var_fiber_buf_size,
		acl_var_fiber_rw_timeout, ACL_VSTREAM_TYPE_SOCK);

	if (acl_getsockname(fd, local, sizeof(local)) == 0)
		acl_vstream_set_local(cstream, local);
	if (acl_getpeername(fd, remote, sizeof(remote)) == 0)
		acl_vstream_set_peer(cstream, remote);

	acl_fiber_create(fiber_client, cstream, acl_var_fiber_stack_size);

	return 0;
}

static int dispatch_report(ACL_VSTREAM *conn)
{
	char buf[256];

	snprintf(buf, sizeof(buf), "count=%d&used=%u&pid=%u&type=%s"
		"&max_threads=%d&curr_threads=%d&busy_threads=%d&qlen=0\r\n",
		__nclients, __nused, (unsigned) getpid(),
		acl_var_fiber_dispatch_type, 1, 1, 1);

	if (acl_vstream_writen(conn, buf, strlen(buf)) == ACL_VSTREAM_EOF) {
		acl_msg_warn("%s(%d), %s: write to master_dispatch(%s) failed",
			__FILE__, __LINE__, __FUNCTION__,
			acl_var_fiber_dispatch_addr);
		return -1;
	}

	return 0;
}

static void dispatch_poll(ACL_VSTREAM *conn)
{
	struct pollfd pfd;
	time_t last = time(NULL);

	memset(&pfd, 0, sizeof(pfd));
	pfd.fd = ACL_VSTREAM_SOCK(conn);
	pfd.events = POLLIN;

	while (!__server_stopping) {
		int n = poll(&pfd, 1, 1000);
		if (n < 0) {
			acl_msg_error("%s(%d), %s: poll error %s",
				__FILE__, __LINE__, __FUNCTION__,
				acl_last_serror());
			break;
		}

		if (n > 0 && pfd.revents & POLLIN) {
			if (dispatch_receive(ACL_VSTREAM_SOCK(conn)) < 0)
				break;

			pfd.revents = 0;
		}

		if (time(NULL) - last >= 1) {
			if (dispatch_report(conn) < 0)
				break;

			last = time(NULL);
		}
	}
}

static void fiber_dispatch(ACL_FIBER *fiber, void *ctx acl_unused)
{
	ACL_VSTREAM *conn;

	if (!acl_var_fiber_dispatch_addr || !*acl_var_fiber_dispatch_addr)
		return;

	while (!__server_stopping) {
		conn = acl_vstream_connect(acl_var_fiber_dispatch_addr,
				ACL_BLOCKING, 0, 0, 4096);
		if (conn == NULL) {
			acl_msg_warn("%s(%d), %s: connect %s error %s",
				__FILE__, __LINE__, __FUNCTION__,
				acl_var_fiber_dispatch_addr, acl_last_serror());
			acl_fiber_sleep(1);
			continue;
		}

		acl_msg_info("%s(%d), %s: connect %s ok",
			__FILE__, __LINE__, __FUNCTION__,
			acl_var_fiber_dispatch_addr);

		dispatch_poll(conn);

		acl_vstream_close(conn);
	}

	acl_msg_info("%s(%d), %s: fiber-%d exit now", __FILE__, __LINE__,
		__FUNCTION__, acl_fiber_id(fiber));
}

static void fiber_accept_main(ACL_FIBER *fiber, void *ctx)
{
	ACL_VSTREAM *sstream = (ACL_VSTREAM *) ctx, *cstream;
	char  ip[64];

	while (!__server_stopping) {
		cstream = acl_vstream_accept(sstream, ip, sizeof(ip));
		if (cstream != NULL) {
			acl_fiber_create(fiber_client, cstream,
				acl_var_fiber_stack_size);
			continue;
		}

#if ACL_EAGAIN == ACL_EWOULDBLOCK
		if (errno == ACL_EAGAIN || errno == ACL_EINTR)
#else
		if (errno == ACL_EAGAIN || errno == ACL_EWOULDBLOCK
			|| errno == ACL_EINTR)
#endif
			continue;

		acl_msg_warn("accept connection: %s(%d, %d), stoping ...",
			acl_last_serror(), errno, ACL_EAGAIN);
		server_abort(fiber_running());
	}

	acl_msg_info("%s(%d), %s: fiber-%d exit now", __FILE__, __LINE__,
		__FUNCTION__, acl_fiber_id(fiber));
	acl_vstream_close(sstream);
}

#ifdef ACL_UNIX

static ACL_VSTREAM **server_daemon_open(int count, int fdtype)
{
	const char *myname = "server_daemon_open";
	ACL_VSTREAM *sstream, **sstreams;
	ACL_SOCKET fd;
	int i;

	/* socket count is as same listen_fd_count in parent process */

	sstreams = (ACL_VSTREAM **)
		acl_mycalloc(count + 1, sizeof(ACL_VSTREAM *));

	for (i = 0; i < count + 1; i++)
		sstreams[i] = NULL;

	i = 0;
	fd = ACL_MASTER_LISTEN_FD;
	for (; fd < ACL_MASTER_LISTEN_FD + count; fd++) {
		sstream = acl_vstream_fdopen(fd, O_RDWR,
				acl_var_fiber_buf_size,
				acl_var_fiber_rw_timeout, fdtype);
		if (sstream == NULL)
			acl_msg_fatal("%s(%d)->%s: stream null, fd = %d",
				__FILE__, __LINE__, myname, fd);

		acl_close_on_exec(fd, ACL_CLOSE_ON_EXEC);
		acl_fiber_create(fiber_accept_main, sstream, STACK_SIZE);
		sstreams[i++] = sstream;
	}

	acl_fiber_create(fiber_monitor_master, ACL_MASTER_STAT_STREAM, STACK_SIZE);

	acl_close_on_exec(ACL_MASTER_STATUS_FD, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_READ, ACL_CLOSE_ON_EXEC);
	acl_close_on_exec(ACL_MASTER_FLOW_WRITE, ACL_CLOSE_ON_EXEC);

	return sstreams;
}

#endif

static ACL_VSTREAM **server_alone_open(const char *addrs)
{
	const char   *myname = "server_alone_open";
	ACL_ARGV*     tokens = acl_argv_split(addrs, ";,| \t");
	ACL_ITER      iter;
	int           i;
	ACL_VSTREAM **streams = (ACL_VSTREAM **)
		acl_mycalloc(tokens->argc + 1, sizeof(ACL_VSTREAM *));

	for (i = 0; i < tokens->argc + 1; i++)
		streams[i] = NULL;

	i = 0;
	acl_foreach(iter, tokens) {
		const char* addr = (const char*) iter.data;
		ACL_VSTREAM* sstream = acl_vstream_listen(addr, 128);
		if (sstream == NULL) {
			acl_msg_error("%s(%d): listen %s error(%s)",
				myname, __LINE__, addr, acl_last_serror());
			exit(1);
		}

		streams[i++] = sstream;

		acl_fiber_create(fiber_accept_main, sstream, STACK_SIZE);
	}

	acl_argv_free(tokens);
	return streams;
}

static void open_service_log(void)
{
	/* first, close the master's log */
#ifdef ACL_UNIX
	master_log_close();
#endif

	/* second, open the service's log */
	acl_msg_open(acl_var_fiber_log_file, acl_var_fiber_procname);

	if (acl_var_fiber_log_debug && *acl_var_fiber_log_debug
		&& acl_var_fiber_max_debug >= 100)
	{
		acl_debug_init2(acl_var_fiber_log_debug,
			acl_var_fiber_max_debug);
	}
}

static void server_init(const char *procname)
{
	const char *myname = "server_init";
	static int inited = 0;
	const char* ptr;

	if (inited)
		return;

	inited = 1;

	if (procname == NULL || *procname == 0)
		acl_msg_fatal("%s(%d); procname null", myname, __LINE__);

	/*
	 * Don't die when a process goes away unexpectedly.
	 */
#ifdef SIGPIPE
	signal(SIGPIPE, SIG_IGN);
#endif

	/*
	 * Don't die for frivolous reasons.
	 */
#ifdef SIGXFSZ
	signal(SIGXFSZ, SIG_IGN);
#endif

	/*
	 * May need this every now and then.
	 */
#ifdef ACL_UNIX
	acl_var_fiber_pid = getpid();
#elif defined(ACL_WINDOWS)
	acl_var_fiber_pid = _getpid();
#else
	acl_var_fiber_pid = 0;
#endif
	acl_var_fiber_procname = acl_mystrdup(acl_safe_basename(procname));

	ptr = acl_getenv("SERVICE_LOG");
	if ((ptr = acl_getenv("SERVICE_LOG")) != NULL && *ptr != 0)
		acl_var_fiber_log_file = acl_mystrdup(ptr);
	else {
		acl_var_fiber_log_file = acl_mystrdup("acl_master.log");
		acl_msg_info("%s(%d)->%s: can't get SERVICE_LOG's env value,"
			" use %s log", __FILE__, __LINE__, myname,
			acl_var_fiber_log_file);
	}

	acl_get_app_conf_int_table(__conf_int_tab);
	acl_get_app_conf_str_table(__conf_str_tab);

#ifdef ACL_UNIX
	acl_master_vars_init(acl_var_fiber_buf_size, acl_var_fiber_rw_timeout);
#endif

	if (__deny_info == NULL)
		__deny_info = acl_var_fiber_deny_banner;
	if (acl_var_fiber_access_allow && *acl_var_fiber_access_allow)
		acl_access_add(acl_var_fiber_access_allow, ", \t", ":");
}

static void usage(int argc, char * argv[])
{
	int   i;
	const char *service_name;

	if (argc <= 0)
		acl_msg_fatal("%s(%d): argc(%d) invalid",
			__FILE__, __LINE__, argc);

	service_name = acl_safe_basename(argv[0]);

	for (i = 0; i < argc; i++)
		acl_msg_info("argv[%d]: %s", i, argv[i]);

	acl_msg_info("usage: %s -h[help]"
		" -c [use chroot]"
		" -n service_name"
		" -s socket_count"
		" -t transport"
		" -u [use setgid initgroups setuid]"
		" -v [on acl_msg_verbose]"
		" -f conf_file"
		" -L listen_addrs",
		service_name);
}

static int __first_name;
static va_list __ap_dest;

static void fiber_main(ACL_FIBER *fiber acl_unused, void *ctx acl_unused)
{
	const char *myname = "fiber_main";
	const char *service_name = acl_safe_basename(__argv[0]);
	char *root_dir = NULL, *user = NULL, *addrs = NULL;
	int   c, fdtype = 0, socket_count = 1, name = -1000;
	char *generation, conf_file[1024];
	void *pre_jail_ctx = NULL, *post_init_ctx = NULL;
	ACL_MASTER_SERVER_INIT_FN pre_jail = NULL;
	ACL_MASTER_SERVER_INIT_FN post_init = NULL;

	master_log_open(__argv[0]);

	conf_file[0] = 0;

	while ((c = getopt(__argc, __argv, "hc:n:s:t:uvf:L:")) > 0) {
		switch (c) {
		case 'h':
			usage(__argc, __argv);
			exit (0);
		case 'f':
			acl_app_conf_load(optarg);
			ACL_SAFE_STRNCPY(conf_file, optarg, sizeof(conf_file));
			break;
		case 'c':
			root_dir = "setme";
			break;
		case 'n':
			service_name = optarg;
			break;
		case 's':
			socket_count = atoi(optarg);
			break;
		case 'u':
			user = "setme";
			break;
		case 't':
			/* deprecated, just go through */
			break;
		case 'v':
			acl_msg_verbose++;
			break;
		case 'L':
			addrs = optarg;
			break;
		default:
			break;
		}
	}

	if (conf_file[0] == 0)
		acl_msg_info("%s(%d)->%s: no configure file",
			__FILE__, __LINE__, myname);
	else
		acl_msg_info("%s(%d)->%s: configure file = %s", 
			__FILE__, __LINE__, myname, conf_file);

	ACL_SAFE_STRNCPY(__service_name, service_name, sizeof(__service_name));

	if (addrs && *addrs)
		__daemon_mode = 0;
	else
		__daemon_mode = 1;

	/*******************************************************************/

	/* Application-specific initialization. */

	/* load configure, set signal */
	server_init(__argv[0]);

	name = __first_name;
	for (; name != ACL_APP_CTL_END; name = va_arg(__ap_dest, int)) {
		switch (name) {
		case ACL_MASTER_SERVER_BOOL_TABLE:
			acl_get_app_conf_bool_table(
				va_arg(__ap_dest, ACL_CONFIG_BOOL_TABLE *));
			break;
		case ACL_MASTER_SERVER_INT_TABLE:
			acl_get_app_conf_int_table(
				va_arg(__ap_dest, ACL_CONFIG_INT_TABLE *));
			break;
		case ACL_MASTER_SERVER_INT64_TABLE:
			acl_get_app_conf_int64_table(
				va_arg(__ap_dest, ACL_CONFIG_INT64_TABLE *));
			break;
		case ACL_MASTER_SERVER_STR_TABLE:
			acl_get_app_conf_str_table(
				va_arg(__ap_dest, ACL_CONFIG_STR_TABLE *));
			break;
		case ACL_MASTER_SERVER_PRE_INIT:
			pre_jail = va_arg(__ap_dest, ACL_MASTER_SERVER_INIT_FN);
			break;
		case ACL_MASTER_SERVER_POST_INIT:
			post_init = va_arg(__ap_dest, ACL_MASTER_SERVER_INIT_FN);
			break;
		case ACL_MASTER_SERVER_EXIT:
			__service_onexit =
				va_arg(__ap_dest, ACL_MASTER_SERVER_EXIT_FN);
			break;
		case ACL_MASTER_SERVER_DENY_INFO:
			__deny_info = acl_mystrdup(va_arg(__ap_dest, const char*));
			break;
		default:
			acl_msg_fatal("%s: bad name(%d)", myname, name);
			break;
		}
	}

	/*******************************************************************/

	if (root_dir)
		root_dir = acl_var_fiber_queue_dir;

	if (user)
		user = acl_var_fiber_owner;

	/* Retrieve process generation from environment. */
	if ((generation = getenv(ACL_MASTER_GEN_NAME)) != 0) {
		if (!acl_alldig(generation))
			acl_msg_fatal("bad generation: %s", generation);
		sscanf(generation, "%o", &__server_generation);
		if (acl_msg_verbose)
			acl_msg_info("process generation: %s (%o)",
				generation, __server_generation);
	}

	/*******************************************************************/

	/* Run pre-jail initialization. */
	if (__daemon_mode && *acl_var_fiber_queue_dir
		&& chdir(acl_var_fiber_queue_dir) < 0)
	{
		acl_msg_fatal("chdir(\"%s\"): %s", acl_var_fiber_queue_dir,
			acl_last_serror());
	}

	/* open all listen streams */

	if (__daemon_mode == 0)
		__sstreams = server_alone_open(addrs);
#ifdef ACL_UNIX
	else if (socket_count <= 0)
		acl_msg_fatal("%s(%d): invalid socket_count: %d",
			myname, __LINE__, socket_count);
	else
		__sstreams = server_daemon_open(socket_count, fdtype);
#else
	else
		acl_msg_fatal("%s(%d): addrs NULL", myname, __LINE__);
#endif

	if (acl_var_fiber_dispatch_addr && *acl_var_fiber_dispatch_addr)
		acl_fiber_create(fiber_dispatch, NULL, STACK_SIZE);

	if (acl_var_fiber_use_limit > 0)
		acl_fiber_create(fiber_monitor_used, NULL, STACK_SIZE);

	if (acl_var_acl_fiber_idle_limit > 0)
		acl_fiber_create(fiber_monitor_idle, NULL, STACK_SIZE);

	if (pre_jail)
		pre_jail(pre_jail_ctx);

#ifdef ACL_UNIX
	if (user && *user)
		acl_chroot_uid(root_dir, user);
#endif

	/* open the server's log */
	open_service_log();

#ifdef ACL_UNIX
	/* if enable dump core when program crashed ? */
	if (acl_var_fiber_enable_core)
		acl_set_core_limit(0);
#endif

	/* Run post-jail initialization. */
	if (post_init)
		post_init(post_init_ctx);

	acl_msg_info("%s(%d), %s daemon started, log: %s",
		myname, __LINE__, __argv[0], acl_var_fiber_log_file);
}

void acl_fiber_server_main(int argc, char *argv[],
	void (*service)(ACL_VSTREAM*, void*), void *ctx, int name, ...)
{
	va_list ap;

	__argc = argc;
	__argv = argv;

	/* Set up call-back info. */
	__service     = service;
	__service_ctx = ctx;

	__first_name = name;
	va_start(ap, name);
	va_copy(__ap_dest, ap);
	va_end(ap);

	acl_fiber_create(fiber_main, NULL, STACK_SIZE);
	acl_fiber_schedule();
}
