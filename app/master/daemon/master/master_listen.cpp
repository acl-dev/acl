#include "stdafx.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

/* Application-specific. */

#include "master_params.h"
#include "master.h"

/* listen on inet/unix socket */

static int master_listen_sock(ACL_MASTER_SERV *serv)
{
	const char *myname = "master_listen_sock";
	int   i, service_type, qlen;
	ACL_ITER iter;

	qlen = serv->max_qlen > acl_var_master_proc_limit
		? serv->max_qlen : acl_var_master_proc_limit;
	if (qlen < 128) {
		acl_msg_warn("%s(%d): qlen(%d) too small, use 128 now",
			myname, __LINE__, qlen);
		qlen = 128;
	}

	if (serv->listen_fd_count != acl_array_size(serv->addrs)) {
		acl_msg_error("%s(%d): listen_fd_count(%d) != addrs's size(%d)",
			myname, __LINE__, serv->listen_fd_count,
			acl_array_size(serv->addrs));
		return -1;
	}

	i = 0;
	acl_foreach(iter, serv->addrs) {
		ACL_MASTER_ADDR *addr = (ACL_MASTER_ADDR*) iter.data;
		switch (addr->type) {
		case ACL_MASTER_SERV_TYPE_INET:
			serv->listen_fds[i] = acl_inet_listen(
				addr->addr, qlen, serv->inet_flags);
			if (serv->listen_fds[i] != ACL_SOCKET_INVALID
				&& serv->defer_accept > 0) {
				acl_tcp_defer_accept(serv->listen_fds[i],
					serv->defer_accept);
			}
			service_type = ACL_VSTREAM_TYPE_LISTEN_INET;
			break;
		case ACL_MASTER_SERV_TYPE_UNIX:
			if (acl_var_master_limit_privilege)
				acl_set_eugid(acl_var_master_owner_uid,
					acl_var_master_owner_gid);
			serv->listen_fds[i] = acl_unix_listen(
				addr->addr, qlen, serv->inet_flags);
			if (acl_var_master_limit_privilege)
				acl_set_ugid(getuid(), getgid());

			service_type = ACL_VSTREAM_TYPE_LISTEN_UNIX;
			break;
		default:
			service_type = 0; /* avoid compile warning */
			serv->listen_fds[i] = ACL_SOCKET_INVALID;
			acl_msg_error("invalid type: %d, addr: %s",
				addr->type, addr->addr);
			break;
		}

		if (serv->listen_fds[i] == ACL_SOCKET_INVALID) {
			acl_msg_error("%s(%d), %s: listen on %s error %s",
				__FILE__, __LINE__, myname,
				addr->addr, strerror(errno));
			continue;
		}

		acl_close_on_exec(serv->listen_fds[i], ACL_CLOSE_ON_EXEC);

		serv->listen_streams[i] = acl_vstream_fdopen(
			serv->listen_fds[i], O_RDONLY, acl_var_master_buf_size,
			acl_var_master_rw_timeout, service_type);

		acl_msg_info("%s(%d), %s: listen on: %s, qlen: %d",
			__FILE__, __LINE__, myname, addr->addr, qlen);
		i++;
	}

	if (i < serv->listen_fd_count) {
		acl_msg_warn("%s(%d), %s: not all listeners were ok!",
			__FILE__, __LINE__, myname);
		serv->listen_fd_count = i;
	}

	return serv->listen_fd_count;
}

static int master_listen_inet(ACL_MASTER_SERV *serv)
{
	const char *myname = "master_listen_inet";
	int   qlen;

	qlen = serv->max_qlen > acl_var_master_proc_limit
		? serv->max_qlen : acl_var_master_proc_limit;
	if (qlen < 128) {
		acl_msg_warn("%s(%d), %s: qlen(%d) too small, use 128 now",
			__FILE__, __LINE__, myname, qlen);
		qlen = 128;
	}

	serv->listen_fds[0] = acl_inet_listen(serv->name, qlen, serv->inet_flags);
	if (serv->listen_fds[0] == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d)->%s: listen on addr(%s) error(%s)",
			__FILE__, __LINE__, myname, serv->name, strerror(errno));
		return 0;
	}

	if (serv->defer_accept > 0)
		acl_tcp_defer_accept(serv->listen_fds[0], serv->defer_accept);

	serv->listen_streams[0] = acl_vstream_fdopen(serv->listen_fds[0],
		O_RDONLY, acl_var_master_buf_size,
		acl_var_master_rw_timeout, ACL_VSTREAM_TYPE_LISTEN_INET);

	acl_close_on_exec(serv->listen_fds[0], ACL_CLOSE_ON_EXEC);
	acl_msg_info("%s(%d), %s: listen on inet: %s, qlen: %d",
		__FILE__, __LINE__, myname, serv->name, qlen);

	return 1;
}

static int master_bind_udp(ACL_MASTER_SERV *serv)
{
	const char *myname = "master_bind_udp";
	ACL_ITER iter;
	int   i = 0;

#ifdef SO_REUSEPORT
	if ((serv->inet_flags & ACL_INET_FLAG_REUSEPORT) != 0) {
		serv->listen_fd_count = 0;
		acl_msg_info("%s(%d): master_reuseport set", myname, __LINE__);
		return 0;
	}
#endif

	if (serv->listen_fd_count != acl_array_size(serv->addrs)) {
		acl_msg_error("%s(%d): listen_fd_count(%d) != addrs's size(%d)",
			myname, __LINE__, serv->listen_fd_count,
			acl_array_size(serv->addrs));
		return -1;
	}

	acl_foreach(iter, serv->addrs) {
		ACL_MASTER_ADDR *addr = (ACL_MASTER_ADDR*) iter.data;
		switch (addr->type) {
		case ACL_MASTER_SERV_TYPE_UDP:
		case ACL_MASTER_SERV_TYPE_INET:
		case ACL_MASTER_SERV_TYPE_UNIX:
		case ACL_MASTER_SERV_TYPE_SOCK:
			serv->listen_streams[i] = acl_vstream_bind(addr->addr,
				acl_var_master_rw_timeout, serv->inet_flags);
			break;
		default:
			acl_msg_error("%s(%d), %s: invalid type: %d, addr: %s",
				__FILE__, __LINE__, myname,
				addr->type, addr->addr);
			serv->listen_streams[i] = NULL;
			break;
		}

		if (serv->listen_streams[i] == NULL) {
			acl_msg_error("%s(%d), %s: bind %s error %s",
				__FILE__, __LINE__, myname,
				addr->addr, strerror(errno));
			continue;
		}

		serv->listen_fds[i] = ACL_VSTREAM_SOCK(serv->listen_streams[i]);
		acl_close_on_exec(serv->listen_fds[i], ACL_CLOSE_ON_EXEC);
		acl_msg_info("%s(%d), %s: bind on %s ok",
			__FILE__, __LINE__, myname, addr->addr);
		i++;
	}

	if (i == 0) {
		acl_msg_error("%s(%d), %s: bind failed for all interface",
			__FILE__, __LINE__, myname);
		return -1;
	}

	if (i < serv->listen_fd_count) {
		acl_msg_warn("%s(%d), %s: not all listeners were ok!",
			__FILE__, __LINE__, myname);
		serv->listen_fd_count = i;
	}

	return serv->listen_fd_count;
}

static int master_listen_unix(ACL_MASTER_SERV *serv)
{
	const char *myname = "master_listen_unix";
	int   qlen;

	qlen = serv->max_qlen > acl_var_master_proc_limit
		? serv->max_qlen : acl_var_master_proc_limit;
	if (qlen < 128) {
		acl_msg_warn("%s(%d): qlen(%d) too small, use 128 now",
			myname, __LINE__, qlen);
		qlen = 128;
	}

	if (acl_var_master_limit_privilege)
		acl_set_eugid(acl_var_master_owner_uid, acl_var_master_owner_gid);
	serv->listen_fds[0] = acl_unix_listen(serv->name, qlen, serv->inet_flags);
	if (serv->listen_fds[0] == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d), %s: listen on addr(%s) error(%s)",
			__FILE__, __LINE__, myname, serv->name, strerror(errno));
		return 0;
	}

	serv->listen_streams[0] = acl_vstream_fdopen(serv->listen_fds[0],
		O_RDONLY, acl_var_master_buf_size,
		acl_var_master_rw_timeout, ACL_VSTREAM_TYPE_LISTEN_UNIX);
	acl_close_on_exec(serv->listen_fds[0], ACL_CLOSE_ON_EXEC);
	if (acl_var_master_limit_privilege)
		acl_set_ugid(getuid(), getgid());
	acl_msg_info("%s(%d), %s: listen on domain socket: %s, qlen: %d",
		__FILE__, __LINE__, myname, serv->name, qlen);

	return 1;
}

static int master_listen_fifo(ACL_MASTER_SERV *serv)
{
	const char *myname = "master_listen_fifo";

	if (acl_var_master_limit_privilege)
		acl_set_eugid(acl_var_master_owner_uid, acl_var_master_owner_gid);
	serv->listen_fds[0] = acl_fifo_listen(serv->name, 0622, ACL_NON_BLOCKING);
	if (serv->listen_fds[0] == ACL_SOCKET_INVALID) {
		acl_msg_error("%s(%d), %s: listen on name(%s) error(%s)",
			__FILE__, __LINE__, myname, serv->name, strerror(errno));
		return 0;
	}

	serv->listen_streams[0] = acl_vstream_fdopen(serv->listen_fds[0],
		O_RDONLY, acl_var_master_buf_size,
		acl_var_master_rw_timeout, ACL_VSTREAM_TYPE_LISTEN);
	acl_close_on_exec(serv->listen_fds[0], ACL_CLOSE_ON_EXEC);
	if (acl_var_master_limit_privilege)
		acl_set_ugid(getuid(), getgid());
	acl_msg_info("%s(%d), %s: listen on fifo socket: %s",
		__FILE__, __LINE__, myname, serv->name);
	return 1;
}

/* acl_master_listen_init - enable connection requests */

int acl_master_listen_init(ACL_MASTER_SERV *serv)
{
	const char *myname = "acl_master_listen_init";

	/*
	 * Find out what transport we should use, then create one or
	 * more listener sockets. Make the listener sockets non-blocking,
	 * so that child processes don't block in accept() when multiple
	 * processes are selecting on the same socket and only one of
	 * them gets the connection.
	 */

	switch (serv->type) {

	/*
	 * SOCK: INET/UNIX domain or stream listener endpoints
	 */
	case ACL_MASTER_SERV_TYPE_SOCK:
		return master_listen_sock(serv) > 0 ? 0 : -1;

	/*      
	 * INET-domain listener endpoints can be wildcarded (the default) or
	 * bound to specific interface addresses.
	 */
	case ACL_MASTER_SERV_TYPE_INET:
		return master_listen_inet(serv) > 0 ? 0 : -1;

	/*
	 * UDP socket endponts
	 */
	case ACL_MASTER_SERV_TYPE_UDP:
		return master_bind_udp(serv) >= 0 ? 0 : -1;

	/*
	 * UNIX-domain or stream listener endpoints always come as singlets.
	 */
	case ACL_MASTER_SERV_TYPE_UNIX:
		return master_listen_unix(serv) > 0 ? 0 : -1;

	/*
	 * FIFO listener endpoints always come as singlets.
	 */
	case ACL_MASTER_SERV_TYPE_FIFO:
		return master_listen_fifo(serv) > 0 ? 0 : -1;

	default:
		acl_msg_error("%s(%d): unknown service type: %d",
			myname, __LINE__, serv->type);
		return -1; /* dummy */
	}
}

/* acl_master_listen_cleanup - disable connection requests */

void acl_master_listen_cleanup(ACL_MASTER_SERV *serv)
{
	const char *myname = "acl_master_listen_cleanup";
	int     i;

	/*
	 * XXX The listen socket is shared with child processes. Closing the
	 * socket in the master process does not really disable listeners in
	 * child processes. There seems to be no documented way to turn off a
	 * listener. The 4.4BSD shutdown(2) man page promises an ENOTCONN error
	 * when shutdown(2) is applied to a socket that is not connected.
	 */
	for (i = 0; i < serv->listen_fd_count; i++) {
		if (close(serv->listen_fds[i]) < 0)
			acl_msg_warn("%s: close listener socket %d: %s",
				myname, serv->listen_fds[i], strerror(errno));
		serv->listen_fds[i] = -1;
		acl_vstream_free(serv->listen_streams[i]);
		serv->listen_streams[i] = NULL;
	}
}
