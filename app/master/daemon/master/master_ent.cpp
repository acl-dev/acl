#include "stdafx.h"

/* Application specific */
#include "master_params.h"
#include "master_pathname.h"
#include "master.h"

#define EQ		!strcasecmp
#define STR		acl_vstring_str
#define	STR_SAME	!strcasecmp

static char *__services_path = NULL;	/* dir name of config files */
static ACL_SCAN_DIR *__scan = NULL;

const char *__config_file_ptr = NULL;

static const char __master_blanks[] = " \t\r\n";  /* field delimiters */

void acl_set_master_service_path(const char *path)
{
	if (__services_path != 0)
		acl_myfree(__services_path);
	__services_path = acl_mystrdup(path);
}

void acl_master_ent_begin()
{
	const char *myname = "acl_master_ent_begin";

	if (__services_path == NULL)
		acl_msg_fatal("%s(%d), %s: master_path is null",
			__FILE__, __LINE__, myname);

	if (__scan)
		acl_master_ent_end();

	__scan = acl_scan_dir_open(__services_path, acl_var_master_scan_subdir);
	if (__scan == NULL)
		acl_msg_fatal("%s(%d), %s: open dir(%s) err, serr=%s",
			__FILE__, __LINE__, myname,
			__services_path, strerror(errno));
}

void acl_master_ent_end()
{
	if (__scan)
		acl_scan_dir_close(__scan);
	__scan = NULL;
}

/* error_with_context - print error with file/line context */

static void error_with_context(const char *format,...)
{
	const char *myname = "error_with_context";
	ACL_VSTRING *vp = acl_vstring_alloc(100);
	va_list ap;

	if (__services_path == 0)
		acl_msg_panic("%s: no configuration file specified", myname);

	va_start(ap, format);
	acl_vstring_vsprintf(vp, format, ap);
	va_end(ap);
	acl_msg_error("%s: file %s: %s", __services_path,
		 __config_file_ptr ? __config_file_ptr : "why?", STR(vp));
}

/* error_invalid_field - report invalid field value */

static void error_invalid_field(const char *name, const char *value)
{
	error_with_context("field \"%s\": bad value: \"%s\"", name, value);
}

/* get_str_ent - extract string field */

static const char *get_str_ent(ACL_XINETD_CFG_PARSER *xcp,
	const char *name, const char *def_val)
{
	const char *value;

	value = acl_xinetd_cfg_get(xcp, name);
	if (value == 0) {
		if (def_val)
			return def_val;
		error_with_context("missing \"%s\" field", name);
		return NULL;
	}

	if (strcmp(value, "-") == 0) {
		if (def_val != 0)
			return def_val;
		error_with_context("field \"%s\" has no default value", name);
		return NULL;
	} else 
		return value;
}

/* get_bool_ent - extract boolean field */

static int get_bool_ent(ACL_XINETD_CFG_PARSER *xcp,
	const char *name, const char *def_val)
{
	const char *value;

	value = acl_xinetd_cfg_get(xcp, name);
	if (value == 0) {
		if (def_val == NULL) {
			error_with_context("missing \"%s\" field", name);
			return 0;
		}
		if (EQ(def_val, "y"))
			return 1;
		else if (EQ(def_val, "n"))
			return 0;
		else {
			error_invalid_field(name, value);
			return 0;
		}
	}

	if (EQ(value, "y") || EQ(value, "yes")
		|| EQ(value, "on") || EQ(value, "true")) {

		return 1;
	} else if (EQ(value, "n") || EQ(value, "no")
		|| EQ(value, "off") || EQ(value, "false")) {

		return 0;
	} else {
		error_invalid_field(name, value);
		return 0;
	}

	/* NOTREACHED */
	return 0;
}

/* get_int_ent - extract integer field */

static int get_int_ent(ACL_XINETD_CFG_PARSER *xcp, const char *name,
	const char *def_val, int min_val)
{
	const char *value;
	int   n = 0;

	value = acl_xinetd_cfg_get(xcp, name);
	if (value == 0) {
		if (def_val == NULL) {
			error_with_context("missing \"%s\" field", name);
			return -1;
		}
		if (!ACL_ISDIGIT(*def_val) || (n = atoi(def_val)) < min_val) {
			error_invalid_field(name, def_val);
			return -1;
		}
		return n;
	}

	if (!ACL_ISDIGIT(*value) || (n = atoi(value)) < min_val) {
		error_invalid_field(name, value);
		return -1;
	}

	return n;
}

static int init_listeners(ACL_MASTER_SERV *serv)
{
	const char *myname = "init_listeners";
	int   n;

	/*
	 * Listen socket(s). XXX We pre-allocate storage because the number of
	 * sockets is frozen anyway once we build the command-line vector below.
	 */
	if (serv->listen_fd_count == 0) {
		acl_msg_error("%s(%d), %s: file %s: no valid IP address found",
			__FILE__, __LINE__, myname, __config_file_ptr);
		return -1;
	}

	serv->listen_fds = (int *) acl_mycalloc(
		1, sizeof(int) * serv->listen_fd_count);
	serv->listen_streams = (ACL_VSTREAM **) acl_mycalloc(
		1, sizeof(ACL_VSTREAM *) * serv->listen_fd_count);
	for (n = 0; n < serv->listen_fd_count; n++) {
		serv->listen_fds[n] = -1;
		serv->listen_streams[n] = NULL;
	}

	return 0;
}

/* FIFO service */

static int service_fifo(ACL_XINETD_CFG_PARSER *xcp, ACL_MASTER_SERV *serv)
{
	const char *name = get_str_ent(xcp, ACL_VAR_MASTER_SERV_SERVICE, NULL);
	char private_val = get_bool_ent(xcp, ACL_VAR_MASTER_SERV_PRIVATE, "y");

	if (name == NULL || *name == 0) {
		acl_msg_error("no %s found", ACL_VAR_MASTER_SERV_SERVICE);
		return -1;
	}

	serv->addrs = NULL;
	serv->type  = ACL_MASTER_SERV_TYPE_FIFO;
	serv->name  = acl_master_pathname(acl_var_master_queue_dir,
			private_val ?  ACL_MASTER_CLASS_PRIVATE :
			ACL_MASTER_CLASS_PUBLIC, name);
	serv->listen_fd_count = 1;
	return 0;
}

/* inet service */

static int service_inet(ACL_XINETD_CFG_PARSER *xcp, ACL_MASTER_SERV *serv)
{
	const char *name = get_str_ent(xcp, ACL_VAR_MASTER_SERV_SERVICE, NULL);

	if (name == NULL || *name == 0) {
		acl_msg_error("no %s found", ACL_VAR_MASTER_SERV_SERVICE);
		return -1;
	}

	serv->addrs = NULL;
	serv->type  = ACL_MASTER_SERV_TYPE_INET;
	serv->name  = acl_mystrdup(name);
	serv->listen_fd_count = 1;
	return 0;
}

/* unix service */

static int service_unix(ACL_XINETD_CFG_PARSER *xcp, ACL_MASTER_SERV *serv)
{
	const char *name = get_str_ent(xcp, ACL_VAR_MASTER_SERV_SERVICE, NULL);
	char private_val = get_bool_ent(xcp, ACL_VAR_MASTER_SERV_PRIVATE, "y");

	if (name == NULL || *name == 0) {
		acl_msg_error("no %s found", ACL_VAR_MASTER_SERV_SERVICE);
		return -1;
	}
	serv->addrs = NULL;
	serv->type  = ACL_MASTER_SERV_TYPE_UNIX;
	serv->name  = acl_master_pathname(acl_var_master_queue_dir,
			private_val ?  ACL_MASTER_CLASS_PRIVATE :
			ACL_MASTER_CLASS_PUBLIC, name);
	serv->listen_fd_count = 1;
	return 0;
}

/* Inet/Unix socket service */

static unsigned addr_matched(const ACL_ARGV *tokens, const char *ip)
{
	ACL_ARGV *tokens_addr = acl_argv_split(ip, ".");
	ACL_ITER iter;
	int   i = 0;

	if (tokens_addr->argc != 4) {
		acl_msg_warn("%s(%d), %s: invalid ip: %s",
			__FILE__, __LINE__, __FUNCTION__, ip);
		acl_argv_free(tokens_addr);
		return 0;
	}

	acl_foreach(iter, tokens_addr) {
		const char* ptr = (const char *) iter.data;
		const char* arg = tokens->argv[i];
		if (strcmp(arg, "*") != 0 && strcmp(arg, ptr) != 0) {
			acl_argv_free(tokens_addr);
			return 0;
		}
		i++;
	}

	acl_argv_free(tokens_addr);
	return 1;
}

static unsigned service_inet_expand(ACL_MASTER_SERV *serv, const char *path)
{
	ACL_ARGV *tokens = NULL;
	ACL_IFCONF *ifconf = NULL;
	char  buf[256], *colon;
	int   port;
	ACL_ITER iter;
	unsigned naddr = 0;

#define RETURN(x) do {                     \
	if (tokens != NULL)                \
		acl_argv_free(tokens);     \
	if (ifconf != NULL)                \
		acl_free_ifaddrs(ifconf);  \
	return (x);                        \
} while (0)

	/* xxx.xxx.xxx.xxx:port, xxx.xxx.xxx.*:port ...*/
	snprintf(buf, sizeof(buf), "%s", path);
	colon = strchr(buf, ':');
	if (colon == NULL || colon == buf || *(colon + 1) == 0)
		RETURN(0);
	*colon++ = 0;
	port = atoi(colon);
	if (port < 0) {
		acl_msg_warn("%s(%d), %s: invalid port: %d, path: %s",
			__FILE__, __LINE__, __FUNCTION__, port, path);
		RETURN(0);
	}

	/* format: xxx.xxx.xxx.*, xxx.xxx.*.*, xxx.*.*.* */
	tokens = acl_argv_split(buf, ".");
	if (tokens->argc != 4)
		RETURN(0);

	ifconf = acl_get_ifaddrs();
	if (ifconf == NULL) {
		acl_msg_error("%s(%d), %s: acl_get_ifaddrs error %s",
			__FILE__, __LINE__, __FUNCTION__, acl_last_serror());
		RETURN(0);
	}

	acl_foreach(iter, ifconf) {
		const ACL_IFADDR* ifaddr = (const ACL_IFADDR *) iter.data;

		if (addr_matched(tokens, ifaddr->ip)) {
			ACL_MASTER_ADDR *addr = (ACL_MASTER_ADDR*)
				acl_mycalloc(1, sizeof(ACL_MASTER_ADDR));

			snprintf(buf, sizeof(buf), "%s:%d", ifaddr->ip, port);
			addr->type = ACL_MASTER_SERV_TYPE_INET;
			addr->addr = acl_mystrdup(buf);
			acl_array_append(serv->addrs, addr);
			serv->listen_fd_count++;
			naddr++;
			acl_msg_info("%s(%d), %s: add addr=%s", __FILE__,
				__LINE__, __FUNCTION__, buf);
		}
	}

	return naddr;
}

static int service_sock(ACL_XINETD_CFG_PARSER *xcp, ACL_MASTER_SERV *serv)
{
	/*
	 * Service name. Syntax is transport-specific.
	 */
	const char *name = get_str_ent(xcp, ACL_VAR_MASTER_SERV_SERVICE, NULL);
	char private_val = get_bool_ent(xcp, ACL_VAR_MASTER_SERV_PRIVATE, "y");
	ACL_ARGV *tokens;
	ACL_ITER iter;

	if (name == NULL || *name == 0) {
		acl_msg_error("no %s found", ACL_VAR_MASTER_SERV_SERVICE);
		return -1;
	}

	tokens      = acl_argv_split(name, ",; \t\r\n");
	serv->name  = acl_mystrdup(name);
	serv->type  = ACL_MASTER_SERV_TYPE_SOCK;
	serv->addrs = acl_array_create(1);

	acl_foreach(iter, tokens) {
		const char *path = (const char*) iter.data;

		if (acl_ipv4_addr_valid(path) || acl_alldig(path)
			|| (*path == ':' && acl_alldig(path + 1))) {

			ACL_MASTER_ADDR *addr = (ACL_MASTER_ADDR*)
				acl_mycalloc(1, sizeof(ACL_MASTER_ADDR));

			addr->type = ACL_MASTER_SERV_TYPE_INET;
			addr->addr = acl_mystrdup(path);
			acl_array_append(serv->addrs, addr);
			serv->listen_fd_count++;
			acl_msg_info("%s(%d), %s: add addr=%s", __FILE__,
				__LINE__, __FUNCTION__, path);
		} else if (service_inet_expand(serv, path) == 0) {
			ACL_MASTER_ADDR *addr = (ACL_MASTER_ADDR*)
				acl_mycalloc(1, sizeof(ACL_MASTER_ADDR));

			addr->type = ACL_MASTER_SERV_TYPE_UNIX;
			addr->addr = acl_master_pathname(
				acl_var_master_queue_dir,
				private_val ? ACL_MASTER_CLASS_PRIVATE :
					ACL_MASTER_CLASS_PUBLIC, path);
			acl_array_append(serv->addrs, addr);
			serv->listen_fd_count++;
			acl_msg_info("%s(%d), %s: add addr=%s", __FILE__,
				__LINE__, __FUNCTION__, path);
		}
	}

	acl_argv_free(tokens);

	if (serv->listen_fd_count == 0) {
		acl_msg_error("no invalid addr found in %s", name);
		return -1;
	}

	return 0;
}

/* UDP socket service */

static int service_udp(ACL_XINETD_CFG_PARSER *xcp, ACL_MASTER_SERV *serv)
{
	const char *name = get_str_ent(xcp, ACL_VAR_MASTER_SERV_SERVICE, NULL);
	ACL_ARGV *tokens;
	ACL_ITER iter;

	if (name == NULL || *name == 0) {
		acl_msg_error("no %s found", ACL_VAR_MASTER_SERV_SERVICE);
		return -1;
	}

	tokens      = acl_argv_split(name, ",; \t");
	serv->type  = ACL_MASTER_SERV_TYPE_UDP;
	serv->addrs = acl_array_create(1);
	serv->name  = acl_mystrdup(name);

	acl_foreach(iter, tokens) {
		const char *ptr = (const char*) iter.data;

		if (acl_ipv4_addr_valid(ptr) || acl_alldig(ptr)
			|| (*ptr == ':' && acl_alldig(ptr + 1))) {

			ACL_MASTER_ADDR *addr = (ACL_MASTER_ADDR*)
				acl_mycalloc(1, sizeof(ACL_MASTER_ADDR));

			addr->type = ACL_MASTER_SERV_TYPE_UDP;
			addr->addr = acl_mystrdup(ptr);

			acl_array_append(serv->addrs, addr);
			serv->listen_fd_count++;
		} else if (service_inet_expand(serv, ptr) == 0) {
			ACL_MASTER_ADDR *addr = (ACL_MASTER_ADDR*)
				acl_mycalloc(1, sizeof(ACL_MASTER_ADDR));

			addr->type = ACL_MASTER_SERV_TYPE_UNIX;
			addr->addr = acl_master_pathname(
				acl_var_master_queue_dir,
				ACL_MASTER_CLASS_PUBLIC, ptr);
			acl_array_append(serv->addrs, addr);
			serv->listen_fd_count++;
			acl_msg_info("%s(%d), %s: add addr=%s", __FILE__,
				__LINE__, __FUNCTION__, ptr);
		}
	}

	return 0;
}

/* Transport type: inet (wild-card listen or virtual) or unix. */

static int service_transport(ACL_XINETD_CFG_PARSER *xcp, ACL_MASTER_SERV *serv)
{
	const char *transport;

	transport = get_str_ent(xcp, ACL_VAR_MASTER_SERV_TYPE, (const char *) 0);
	if (transport == NULL || *transport == 0) {
		acl_msg_error("master_service no found");
		return -1;
	}

	serv->defer_accept = get_int_ent(xcp, ACL_VAR_MASTER_SERV_DEFER_ACCEPT,
			ACL_DEF_MASTER_SERV_DEFER_ACCEPT, 0);

	if (STR_SAME(transport, ACL_MASTER_XPORT_NAME_FIFO)) {
		if (service_fifo(xcp, serv) < 0)
			return -1;
	} else if (STR_SAME(transport, ACL_MASTER_XPORT_NAME_UNIX)) {
		if (service_unix(xcp, serv) < 0)
			return -1;
	} else if (STR_SAME(transport, ACL_MASTER_XPORT_NAME_INET)) {
		if (service_inet(xcp, serv) < 0)
			return -1;
	} else if (STR_SAME(transport, ACL_MASTER_XPORT_NAME_SOCK)) {
		if (service_sock(xcp, serv) < 0)
			return -1;
	} else if (STR_SAME(transport, ACL_MASTER_XPORT_NAME_UDP)) {
		if (service_udp(xcp, serv) < 0)
			return -1;
	} else {
		acl_msg_error("unknown master_service: %s", transport);
		return -1;
	}

	if (get_bool_ent(xcp, ACL_VAR_MASTER_SERV_REUSEPORT, "n"))
		serv->inet_flags |= ACL_INET_FLAG_REUSEPORT;
	if (get_bool_ent(xcp, ACL_VAR_MASTER_SERV_FASTOPEN, "n"))
		serv->inet_flags |= ACL_INET_FLAG_FASTOPEN;
	if (get_bool_ent(xcp, ACL_VAR_MASTER_SERV_NBLOCK, "y"))
		serv->inet_flags |= ACL_INET_FLAG_NBLOCK;

	return init_listeners(serv);
}

static void service_control(ACL_XINETD_CFG_PARSER *xcp, ACL_MASTER_SERV *serv)
{
	const char* ptr = get_str_ent(xcp, ACL_VAR_MASETR_SERV_STOP_KILL, "off");
	if (EQ(ptr, "on") || EQ(ptr, "true") || atoi(ptr) > 1)
		serv->flags |= ACL_MASTER_FLAG_STOP_KILL;
	else
		serv->flags &=~ ACL_MASTER_FLAG_STOP_KILL;

	ptr = get_str_ent(xcp, ACL_VAR_MASTER_SERV_STOP_WAIT, "off");
	if (EQ(ptr, "on") || EQ(ptr, "true") || atoi(ptr) > 0)
		serv->flags |= ACL_MASTER_FLAG_STOP_WAIT;
	else
		serv->flags &= ~ACL_MASTER_FLAG_STOP_WAIT;
}

static void service_wakeup_time(ACL_XINETD_CFG_PARSER *xcp,
	ACL_MASTER_SERV *serv)
{
	const char *ptr_const;
	char *ptr, *ptr1;

	/*
	 * Wakeup timer. XXX should we require that var_master_proc_limit == 1?
	 * Right now, the only services that have a wakeup timer also happen
	 * to be the services that have at most one running instance:
	 * local pickup and local delivery.
	 */
	ptr_const = get_str_ent(xcp, ACL_VAR_MASTER_SERV_WAKEUP, "0");

	/*
	 * Find out if the wakeup time is conditional, i.e., wakeup triggers
	 * should not be sent until the service has actually been used.
	 */
	ptr = NULL;
	if (ptr_const) {
		ptr = acl_mystrdup(ptr_const);
		ptr1 = strchr(ptr, '?');
		if (ptr1)
			*ptr1 = 0;
		serv->wakeup_time = atoi(ptr);
	} else {
		ptr1 = NULL;
		serv->wakeup_time = 0;
	}

	if (serv->wakeup_time > 0 && ptr1 != NULL)
		serv->flags |= ACL_MASTER_FLAG_CONDWAKE;
	if (ptr)
		acl_myfree(ptr);
}

static void service_proc(ACL_XINETD_CFG_PARSER *xcp, ACL_MASTER_SERV *serv)
{
	/*
	 * Concurrency limit. Zero means no limit.
	 */

	serv->max_qlen = get_int_ent(xcp, ACL_VAR_MASTER_SERV_MAX_QLEN,
			ACL_DEF_MASTER_SERV_MAX_QLEN, 0);
	serv->max_proc = get_int_ent(xcp, ACL_VAR_MASTER_SERV_MAX_PROC,
			ACL_DEF_MASTER_SERV_MAX_PROC, 0);
	serv->prefork_proc = get_int_ent(xcp, ACL_VAR_MASTER_SERV_PREFORK_PROC,
			ACL_DEF_MASTER_SERV_PREFORK_PROC, 0);
	if (serv->max_proc > 0 && serv->prefork_proc > serv->max_proc)
		serv->prefork_proc = serv->max_proc;

	/*
	 * Idle and total process count.
	 */
	serv->avail_proc = 0;
	serv->total_proc = 0;
}

static int service_args(ACL_XINETD_CFG_PARSER *xcp, ACL_MASTER_SERV *serv,
	const char *path)
{
	const char *myname = "service_args";
	const char *command, *name, *transport, *args, *ptr_const;
	char   *args_buf, *ptr, *cp;
	char  unprivileged, chroot_var; 
	ACL_VSTRING *junk = acl_vstring_alloc(100);

	/*
	 * Privilege level. Default is to restrict process privileges
	 * to those of the mail owner.
	 */
	unprivileged = get_bool_ent(xcp, ACL_VAR_MASTER_SERV_UNPRIV, "y");

	/*
	 * Chroot. Default is to restrict file system access to the mail queue.
	 * xxx: Chroot cannot imply unprivileged service (for example, the
	 * pickup service runs chrooted but needs privileges to open files
	 * as the user).
	 */
	chroot_var = get_bool_ent(xcp, ACL_VAR_MASTER_SERV_CHROOT, "y");

	ptr_const = get_str_ent(xcp, ACL_VAR_MASTER_SERV_OWNER, "");
	if (ptr_const && *ptr_const)
		serv->owner = acl_mystrdup(ptr_const);
	else
		serv->owner = NULL;

	/*
	 * Path to command,
	 */
	command = get_str_ent(xcp, ACL_VAR_MASTER_SERV_COMMAND, (char *) 0);
	if (command == NULL || *command == 0) {
		acl_msg_error("no %s found", ACL_VAR_MASTER_SERV_COMMAND);
		return -1;
	}

	serv->path = acl_concatenate(acl_var_master_daemon_dir, "/",
			command, (char *) 0);

	/*
	 * Notify Address
	 */
	ptr_const = get_str_ent(xcp, ACL_VAR_MASTER_NOTIFY_ADDR, "no");
	if (ptr_const == NULL || strcasecmp(ptr_const, "no") == 0)
		serv->notify_addr = NULL;
	else
		serv->notify_addr = acl_mystrdup(ptr_const);

	ptr_const = get_str_ent(xcp, ACL_VAR_MASTER_NOTIFY_RECIPIENTS, "no");
	if (ptr_const == NULL || strcasecmp(ptr_const, "no") == 0)
		serv->notify_recipients = NULL;
	else
		serv->notify_recipients = acl_mystrdup(ptr_const);

	/*
	 * Command-line vector. Add "-n service_name" when the process name
	 * safe_basename differs from the service name. Always add the transport.
	 */
	serv->args = acl_argv_alloc(0);
	acl_argv_add(serv->args, command, (char *) 0);

	name = get_str_ent(xcp, ACL_VAR_MASTER_SERV_SERVICE, (const char *) 0);
	if (name == NULL || *name == 0) {
		acl_msg_error("no %s found", ACL_VAR_MASTER_SERV_SERVICE);
		return -1;
	}

	/* add "-f configure_file_path" flag */
	acl_argv_add(serv->args, "-f", path, (char *) 0);

	/* copy the configure filepath */
	serv->conf = acl_mystrdup(path);

	/*
	if (serv->max_proc == 1)
		acl_argv_add(serv->args, "-l", (char *) 0);
	if (serv->max_proc == 0)
		acl_argv_add(serv->args, "-z", (char *) 0);
	*/

	if (strcmp(acl_safe_basename(command), name) != 0) {
		char *tmp = acl_concatenate("\"", name, "\"", 0);
		acl_argv_add(serv->args, "-n", tmp, (char *) 0);
		acl_myfree(tmp);
	}

	transport = get_str_ent(xcp, ACL_VAR_MASTER_SERV_TYPE, (const char *) 0);
	if (transport == NULL || *transport == 0) {
		acl_msg_error("no %s found", ACL_VAR_MASTER_SERV_TYPE);
		return -1;
	}

	acl_argv_add(serv->args, "-t", transport, (char *) 0);
	if (acl_msg_verbose)
		acl_argv_add(serv->args, "-v", (char *) 0);
	if (unprivileged)
		acl_argv_add(serv->args, "-u", (char *) 0);
	if (chroot_var)
		acl_argv_add(serv->args, "-c", (char *) 0);
	if (serv->listen_fd_count > 1)
		acl_argv_add(serv->args, "-s", STR(
			acl_vstring_sprintf(junk, "%d", serv->listen_fd_count)),
			(char *) 0);

	/* use myname to avoid fatal error when nothing get */
	args = get_str_ent(xcp, ACL_VAR_MASTER_SERV_ARGS, myname);
	if (args && args != myname) {
		args_buf = acl_mystrdup(args);
		ptr = args_buf;
		while ((cp = acl_mystrtok(&ptr, __master_blanks)) != 0)
			acl_argv_add(serv->args, cp, (char *) 0);
		acl_myfree(args_buf);
	}

	acl_argv_terminate(serv->args);
	acl_vstring_free(junk);
	return 0;
}

static int service_env(ACL_XINETD_CFG_PARSER *xcp, ACL_MASTER_SERV *serv)
{
	ACL_MASTER_NV *nv;
	const char *value;

	serv->children_env = acl_array_create(10);

	/* use the master's logfile as default if no log entry in configure */
	value = get_str_ent(xcp, ACL_VAR_MASTER_SERV_LOG, acl_var_master_log_file);
	if (value == NULL || *value == 0) {
		acl_msg_error("no %s found", ACL_VAR_MASTER_SERV_LOG);
		return -1;
	}

	nv = (ACL_MASTER_NV *) acl_mycalloc(1, sizeof(ACL_MASTER_NV));
	nv->name  = acl_mystrdup("MASTER_LOG");
	nv->value = acl_mystrdup(acl_var_master_log_file);
	(void) acl_array_append(serv->children_env, nv);

	nv = (ACL_MASTER_NV *) acl_mycalloc(1, sizeof(ACL_MASTER_NV));
	nv->name  = acl_mystrdup("SERVICE_LOG");
	nv->value = acl_mystrdup(value);
	(void) acl_array_append(serv->children_env, nv);

	/* 为了保证兼容性 */
	nv = (ACL_MASTER_NV *) acl_mycalloc(1, sizeof(ACL_MASTER_NV));
	nv->name  = acl_mystrdup("LOG");
	nv->value = acl_mystrdup(value);
	(void) acl_array_append(serv->children_env, nv);

	value = get_str_ent(xcp, ACL_VAR_MASTER_SERV_ENV, "-");
	if (value && strcmp(value, "-") != 0) {
		nv = (ACL_MASTER_NV *) acl_mycalloc(1, sizeof(ACL_MASTER_NV));
		nv->name  = acl_mystrdup("SERVICE_ENV");
		nv->value = acl_mystrdup(value);
		(void) acl_array_append(serv->children_env, nv);
	}

	return 0;
}

ACL_MASTER_SERV *acl_master_ent_get()
{
	static char *saved_interfaces = 0;
	ACL_VSTRING *path_buf = acl_vstring_alloc(256);
	const char  *filepath;
	ACL_MASTER_SERV *serv;

	/*
	 * XXX We cannot change the inet_interfaces setting for a running
	 * master process. Listening sockets are inherited by child processes
	 * so that closing and reopening those sockets in the master does not
	 * work. Another problem is that library routines still cache results
	 * that are based on the old inet_interfaces setting. It is too much
	 * trouble to recompute everything.
	 * 
	 * In order to keep our data structures consistent we ignore changes
	 * in inet_interfaces settings, and issue a warning instead.
	 */
	if (saved_interfaces == 0)
		saved_interfaces = acl_mystrdup(acl_var_master_inet_interfaces);

	while (1) {
		filepath = acl_scan_dir_next_file(__scan);
		if (filepath == NULL) {
			acl_vstring_free(path_buf);
			return NULL;
		}

		acl_vstring_sprintf(path_buf, "%s/%s",
			acl_scan_dir_path(__scan), filepath);

		acl_msg_info("%s(%d), %s: load service file = %s",
			__FILE__, __LINE__, __FUNCTION__, STR(path_buf));

		serv = acl_master_ent_load(STR(path_buf));
		if (serv != NULL) {
			acl_vstring_free(path_buf);
			return serv;
		}
	}
}

ACL_MASTER_SERV *acl_master_ent_load(const char *filepath)
{
	ACL_XINETD_CFG_PARSER *xcp = acl_xinetd_cfg_load(filepath);
	ACL_MASTER_SERV *serv;
	const char *ptr;

	if (xcp == NULL) {
		acl_msg_error("%s(%d), %s: load %s error %s", __FILE__,
			__LINE__, __FUNCTION__, filepath, acl_last_serror());
		return NULL;
	}

	ptr = get_str_ent(xcp, ACL_VAR_MASTER_SERV_DISABLE, "yes");
	if (ptr == NULL || strcasecmp(ptr, "yes") == 0) {
		acl_xinetd_cfg_free(xcp);
		return NULL;
	}

	/* Initialize service structure members in order. */
	serv = (ACL_MASTER_SERV *) acl_mycalloc(1, sizeof(ACL_MASTER_SERV));
	serv->next = 0;

	/* Flags member. */
	serv->flags = 0;
	serv->inet_flags = 0;

	if (service_transport(xcp, serv) < 0) {
		acl_master_ent_free(serv);
		return NULL;
	}

	service_wakeup_time(xcp, serv);
	service_proc(xcp, serv);
	service_control(xcp, serv);

	if (service_args(xcp, serv, filepath) < 0) {
		acl_master_ent_free(serv);
		return NULL;
	}
	if (service_env(xcp, serv) < 0) {
		acl_master_ent_free(serv);
		return NULL;
	}

	/* linked for children */
	acl_ring_init(&serv->children);

	/* Backoff time in case a service is broken. */
	serv->throttle_delay = acl_var_master_throttle_time;

	/* Shared channel for child status updates. */
	serv->status_fd[0] = serv->status_fd[1] = -1;

	acl_xinetd_cfg_free(xcp);
	return serv;
}

/* acl_print_master_ent - show service entry contents */

void acl_master_ent_print(ACL_MASTER_SERV *serv)
{
	char  **cpp;

	acl_msg_info("====start service entry");
	acl_msg_info("flags: %d", serv->flags);
	acl_msg_info("name: %s", serv->name);
	acl_msg_info("type: %s",
		serv->type == ACL_MASTER_SERV_TYPE_UNIX ? ACL_MASTER_XPORT_NAME_UNIX :
		serv->type == ACL_MASTER_SERV_TYPE_FIFO ? ACL_MASTER_XPORT_NAME_FIFO :
		serv->type == ACL_MASTER_SERV_TYPE_INET ? ACL_MASTER_XPORT_NAME_INET :
		serv->type == ACL_MASTER_SERV_TYPE_SOCK ? ACL_MASTER_XPORT_NAME_SOCK :
		"unknown transport type");
	acl_msg_info("listen_fd_count: %d", serv->listen_fd_count);
	acl_msg_info("wakeup: %d", serv->wakeup_time);
	acl_msg_info("max_proc: %d", serv->max_proc);
	acl_msg_info("prefork_proc: %d", serv->prefork_proc);
	acl_msg_info("path: %s", serv->path);
	for (cpp = serv->args->argv; *cpp; cpp++)
		acl_msg_info("arg[%d]: %s", (int) (cpp - serv->args->argv), *cpp);
	acl_msg_info("prefork_proc: %d", serv->prefork_proc);
	acl_msg_info("avail_proc: %d", serv->avail_proc);
	acl_msg_info("total_proc: %d", serv->total_proc);
	acl_msg_info("throttle_delay: %d", serv->throttle_delay);
	acl_msg_info("status_fd %d %d", serv->status_fd[0], serv->status_fd[1]);
	acl_msg_info("next: 0x%lx", (long) serv->next);
	acl_msg_info("====end service entry");
}

static void __free_nv_fn(void *arg)
{
	ACL_MASTER_NV *nv;

	if (arg) {
		nv = (ACL_MASTER_NV *) arg;
		acl_myfree(nv->name);
		acl_myfree(nv->value);
		acl_myfree(nv);
	}
}

void acl_master_ent_free(ACL_MASTER_SERV *serv)
{
	/*
	 * Undo what get_master_ent() created.
	 */

	if (serv->addrs != NULL) {
		ACL_ITER iter;
		acl_foreach(iter, serv->addrs) {
			ACL_MASTER_ADDR *addr = (ACL_MASTER_ADDR*) iter.data;
			acl_myfree(addr->addr);
			acl_myfree(addr);
		}
		acl_array_free(serv->addrs, NULL);
	}

	if (serv->name)
		acl_myfree(serv->name);
	if (serv->path)
		acl_myfree(serv->path);
	if (serv->conf)
		acl_myfree(serv->conf);
	if (serv->owner)
		acl_myfree(serv->owner);
	if (serv->notify_addr)
		acl_myfree(serv->notify_addr);
	if (serv->notify_recipients)
		acl_myfree(serv->notify_recipients);
	if (serv->args)
		acl_argv_free(serv->args);
	if (serv->children_env)
		acl_array_destroy(serv->children_env, __free_nv_fn);
	if (serv->listen_fds)
		acl_myfree(serv->listen_fds);
	if (serv->listen_streams)
		acl_myfree(serv->listen_streams);
	acl_myfree(serv);
}

int acl_master_same_name(ACL_MASTER_SERV *serv, const char *name)
{
	const char *sep = ",; \t\r\n";
	ACL_ARGV *tokens_old = acl_argv_split(serv->name, sep);
	ACL_ARGV *tokens_new = acl_argv_split(name, sep);
	int       i, ret;

	if (tokens_old->argc != tokens_new->argc) {
		acl_argv_free(tokens_old);
		acl_argv_free(tokens_new);
		return 0;
	}

	for (i = 0; i < tokens_old->argc; i++) {
		if (strcmp(tokens_new->argv[i], tokens_old->argv[i]) != 0)
			break;
	}

	ret = i == tokens_old->argc ? 1 : 0;

	acl_argv_free(tokens_old);
	acl_argv_free(tokens_new);

	return ret;
}

ACL_MASTER_SERV *acl_master_ent_find(const char *name, int type)
{
	ACL_MASTER_SERV *serv;

	if (acl_var_master_head == NULL) {
		acl_msg_error("%s(%d), %s: acl_var_master_head null",
			__FILE__, __LINE__, __FUNCTION__);
		return NULL;
	}

	for (serv = acl_var_master_head; serv; serv = serv->next) {
		if (serv->type == type && acl_master_same_name(serv, name))
			return serv;
	}

	return NULL;
}
