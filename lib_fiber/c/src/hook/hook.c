#include "stdafx.h"
#include "common.h"
#include "hook.h"

socket_fn     __sys_socket                  = NULL;
socket_fn     *sys_socket                   = NULL;

close_fn      __sys_close                   = NULL;
close_fn      *sys_close                    = NULL;

listen_fn     __sys_listen                  = NULL;
listen_fn     *sys_listen                   = NULL;

accept_fn     __sys_accept                  = NULL;
accept_fn     *sys_accept                   = NULL;

connect_fn    __sys_connect                 = NULL;
connect_fn    *sys_connect                  = NULL;

recv_fn       __sys_recv                    = NULL;
recv_fn       *sys_recv                     = NULL;

recvfrom_fn   __sys_recvfrom                = NULL;
recvfrom_fn   *sys_recvfrom                 = NULL;

send_fn       __sys_send                    = NULL;
send_fn       *sys_send                     = NULL;

sendto_fn     __sys_sendto                  = NULL;
sendto_fn     *sys_sendto                   = NULL;

poll_fn       __sys_poll                    = NULL;
poll_fn       *sys_poll                     = NULL;

select_fn     __sys_select                  = NULL;
select_fn     *sys_select                   = NULL;

getaddrinfo_fn   __sys_getaddrinfo          = NULL;
getaddrinfo_fn   *sys_getaddrinfo           = NULL;

freeaddrinfo_fn  __sys_freeaddrinfo         = NULL;
freeaddrinfo_fn  *sys_freeaddrinfo          = NULL;

gethostbyname_fn __sys_gethostbyname        = NULL;
gethostbyname_fn *sys_gethostbyname         = NULL;

#ifdef SYS_UNIX

sleep_fn    __sys_sleep                     = NULL;
sleep_fn    *sys_sleep                      = NULL;

fcntl_fn    __sys_fcntl                     = NULL;
fcntl_fn    *sys_fcntl                      = NULL;

setsockopt_fn __sys_setsockopt              = NULL;
setsockopt_fn *sys_setsockopt               = NULL;

getsockopt_fn __sys_getsockopt              = NULL;
getsockopt_fn *sys_getsockopt               = NULL;

read_fn     __sys_read                      = NULL;
read_fn     *sys_read                       = NULL;

readv_fn    __sys_readv                     = NULL;
readv_fn    *sys_readv                      = NULL;

recvmsg_fn  __sys_recvmsg                   = NULL;
recvmsg_fn  *sys_recvmsg                    = NULL;

write_fn    __sys_write                     = NULL;
write_fn    *sys_write                      = NULL;

writev_fn   __sys_writev                    = NULL;
writev_fn   *sys_writev                     = NULL;

sendmsg_fn  __sys_sendmsg                   = NULL;
sendmsg_fn  *sys_sendmsg                    = NULL;

# ifdef HAS_MMSG
recvmmsg_fn  __sys_recvmmsg                 = NULL;
recvmmsg_fn  *sys_recvmmsg                  = NULL;

sendmmsg_fn  __sys_sendmmsg                 = NULL;
sendmmsg_fn  *sys_sendmmsg                  = NULL;

# endif

# ifdef __USE_LARGEFILE64
sendfile64_fn __sys_sendfile64              = NULL;
sendfile64_fn *sys_sendfile64               = NULL;
# endif

pread_fn      __sys_pread                   = NULL;
pread_fn      *sys_pread                    = NULL;
pwrite_fn     __sys_pwrite                  = NULL;
pwrite_fn     *sys_pwrite                   = NULL;

# ifdef HAS_EPOLL
epoll_create_fn __sys_epoll_create          = NULL;
epoll_create_fn *sys_epoll_create           = NULL;

epoll_wait_fn   __sys_epoll_wait            = NULL;
epoll_wait_fn   *sys_epoll_wait             = NULL;

epoll_ctl_fn    __sys_epoll_ctl             = NULL;
epoll_ctl_fn    *sys_epoll_ctl              = NULL;
# endif

# ifdef HAS_IO_URING
openat_fn       __sys_openat                = NULL;
openat_fn       *sys_openat                 = NULL;
unlink_fn       __sys_unlink                = NULL;
unlink_fn       *sys_unlink                 = NULL;
# ifdef HAS_STATX
statx_fn        __sys_statx                 = NULL;
statx_fn        *sys_statx                  = NULL;
# endif
# ifdef HAS_RENAMEAT2
renameat2_fn    __sys_renameat2             = NULL;
renameat2_fn    *sys_renameat2              = NULL;
# endif
mkdirat_fn      __sys_mkdirat               = NULL;
mkdirat_fn      *sys_mkdirat                = NULL;
splice_fn       __sys_splice                = NULL;
splice_fn       *sys_splice                 = NULL;
# endif

# ifndef __APPLE__
gethostbyname_r_fn __sys_gethostbyname_r    = NULL;
gethostbyname_r_fn *sys_gethostbyname_r     = NULL;
# endif

#elif defined(SYS_WIN)

WSARecv_fn __sys_WSARecv                     = NULL;
WSARecv_fn *sys_WSARecv                      = NULL;

WSAAccept_fn __sys_WSAAccept                 = NULL;
WSAAccept_fn *sys_WSAAccept                  = NULL;

#endif // SYS_WIN

void WINAPI set_socket_fn(socket_fn *fn)
{
	sys_socket = fn;
}

void WINAPI set_close_fn(close_fn *fn)
{
	sys_close = fn;
}

void WINAPI set_listen_fn(listen_fn *fn)
{
	sys_listen = fn;
}

void WINAPI set_accept_fn(accept_fn *fn)
{
	sys_accept = fn;
}

void WINAPI set_connect_fn(connect_fn *fn)
{
	sys_connect = fn;
}

void WINAPI set_recv_fn(recv_fn *fn)
{
	sys_recv = fn;
}

void WINAPI set_recvfrom_fn(recvfrom_fn *fn)
{
	sys_recvfrom = fn;
}

void WINAPI set_send_fn(send_fn *fn)
{
	sys_send = fn;
}

void WINAPI set_sendto_fn(sendto_fn *fn)
{
	sys_sendto = fn;
}

void WINAPI set_poll_fn(poll_fn *fn)
{
	sys_poll = fn;
}

void WINAPI set_select_fn(select_fn *fn)
{
	sys_select = fn;
}

void WINAPI set_getaddrinfo_fn(getaddrinfo_fn *fn)
{
	sys_getaddrinfo = fn;
}

void WINAPI set_freeaddrinfo_fn(freeaddrinfo_fn *fn)
{
	sys_freeaddrinfo = fn;
}

void WINAPI set_gethostbyname_fn(gethostbyname_fn *fn)
{
	sys_gethostbyname = fn;
}

#if defined(SYS_WIN)
void WINAPI set_WSAAccept_fn(WSAAccept_fn *fn)
{
	sys_WSAAccept = fn;
}

void WINAPI set_WSARecv_fn(WSARecv_fn *fn)
{
	sys_WSARecv = fn;
}
#endif

static void hook_api(void)
{
#ifdef SYS_UNIX

# ifdef MINGW
#  define LOAD_FN(name, type, fn, fp, fatal) do { \
	(fn) = (type) dlsym(RTLD_DEFAULT, name); \
	if ((fn) == NULL) { \
		assert((fatal) != 1); \
	} \
	(fp) = &(fn); \
} while (0)
# else
#  define LOAD_FN(name, type, fn, fp, fatal) do { \
	(fn) = (type) dlsym(RTLD_NEXT, name); \
	if ((fn) == NULL) { \
		const char* e = dlerror(); \
		printf("%s(%d): name=%s not found: %s\r\n", \
			__FUNCTION__, __LINE__, name, e ? e : "unknown"); \
		assert((fatal) != 1); \
	} \
	(fp) = &(fn); \
} while (0)
# endif

	LOAD_FN("socket", socket_fn, __sys_socket, sys_socket, 1);
	LOAD_FN("close", close_fn, __sys_close, sys_close, 1);
	LOAD_FN("listen", listen_fn, __sys_listen, sys_listen, 1);
	LOAD_FN("accept", accept_fn, __sys_accept, sys_accept, 1);
	LOAD_FN("connect", connect_fn, __sys_connect, sys_connect, 1);
	LOAD_FN("setsockopt", setsockopt_fn, __sys_setsockopt, sys_setsockopt, 1);
	LOAD_FN("getsockopt", getsockopt_fn, __sys_getsockopt, sys_getsockopt, 1);
	LOAD_FN("sleep", sleep_fn, __sys_sleep, sys_sleep, 1);
	LOAD_FN("fcntl", fcntl_fn, __sys_fcntl, sys_fcntl, 1);
	LOAD_FN("read", read_fn, __sys_read, sys_read, 1);
	LOAD_FN("readv", readv_fn, __sys_readv, sys_readv, 1);
	LOAD_FN("recv", recv_fn, __sys_recv, sys_recv, 1);
	LOAD_FN("recvfrom", recvfrom_fn, __sys_recvfrom, sys_recvfrom, 1);
	LOAD_FN("recvmsg", recvmsg_fn, __sys_recvmsg, sys_recvmsg, 1);
	LOAD_FN("write", write_fn, __sys_write, sys_write, 1);
	LOAD_FN("writev", writev_fn, __sys_writev, sys_writev, 1);
	LOAD_FN("send", send_fn, __sys_send, sys_send, 1);
	LOAD_FN("sendto", sendto_fn, __sys_sendto, sys_sendto, 1);
	LOAD_FN("sendmsg", sendmsg_fn, __sys_sendmsg, sys_sendmsg, 1);

# ifdef HAS_MMSG
	LOAD_FN("recvmmsg", recvmmsg_fn, __sys_recvmmsg, sys_recvmmsg, 0);
	LOAD_FN("sendmmsg", sendmmsg_fn, __sys_sendmmsg, sys_sendmmsg, 0);
# endif

# ifdef __USE_LARGEFILE64
	LOAD_FN("sendfile64", sendfile64_fn, __sys_sendfile64, sys_sendfile64, 0);
# endif
	LOAD_FN("pread", pread_fn, __sys_pread, sys_pread, 1);
	LOAD_FN("pwrite", pwrite_fn, __sys_pwrite, sys_pwrite, 1);
	LOAD_FN("poll", poll_fn, __sys_poll, sys_poll, 1);
	LOAD_FN("select", select_fn, __sys_select, sys_select, 1);

# ifdef	HAS_EPOLL
	LOAD_FN("epoll_create", epoll_create_fn, __sys_epoll_create, sys_epoll_create, 1);

	LOAD_FN("epoll_wait", epoll_wait_fn, __sys_epoll_wait, sys_epoll_wait, 1);

	LOAD_FN("epoll_ctl", epoll_ctl_fn, __sys_epoll_ctl, sys_epoll_ctl, 1);
# endif // HAS_EPOLL

# ifdef	HAS_IO_URING
	LOAD_FN("openat", openat_fn, __sys_openat, sys_openat, 1);
	LOAD_FN("unlink", unlink_fn, __sys_unlink, sys_unlink, 1);
# ifdef HAS_STATX
	LOAD_FN("statx", statx_fn, __sys_statx, sys_statx, 1);
# endif
# ifdef HAS_RENAMEAT2
	LOAD_FN("renameat2", renameat2_fn, __sys_renameat2, sys_renameat2, 1);
# endif
	LOAD_FN("mkdirat", mkdirat_fn, __sys_mkdirat, sys_mkdirat, 1);
	LOAD_FN("splice", splice_fn, __sys_splice, sys_splice, 1);
# endif

	LOAD_FN("getaddrinfo", getaddrinfo_fn, __sys_getaddrinfo, sys_getaddrinfo, 1);
	LOAD_FN("freeaddrinfo", freeaddrinfo_fn, __sys_freeaddrinfo, sys_freeaddrinfo, 1);
	LOAD_FN("gethostbyname", gethostbyname_fn, __sys_gethostbyname, sys_gethostbyname, 1);

# ifndef __APPLE__
	LOAD_FN("gethostbyname_r", gethostbyname_r_fn, __sys_gethostbyname_r, sys_gethostbyname_r, 1);
# endif
#elif defined(SYS_WIN)
	__sys_socket    = socket;
	sys_socket      = &__sys_socket;

	__sys_listen    = listen;
	sys_listen      = &__sys_listen;

	__sys_accept    = accept;
	sys_accept      = &__sys_accept;

	__sys_connect   = connect;
	sys_connect     = &__sys_connect;

	__sys_close     = closesocket;
	sys_close       = &__sys_close;

	__sys_recv      = recv;
	sys_recv        = &__sys_recv;

	__sys_recvfrom  = recvfrom;
	sys_recvfrom    = &__sys_recvfrom;

	__sys_send      = send;
	sys_send        = &__sys_send;

	__sys_sendto    = sendto;
	sys_sendto      = &__sys_sendto;

	__sys_poll      = WSAPoll;
	sys_poll        = &__sys_poll;

	__sys_select    = select;
	sys_select      = &__sys_select;

	__sys_WSARecv   = WSARecv;
	sys_WSARecv     = &__sys_WSARecv;

	__sys_WSAAccept = WSAAccept;
	sys_WSAAccept   = &__sys_WSAAccept;

	__sys_getaddrinfo   = getaddrinfo;
	sys_getaddrinfo     = &__sys_getaddrinfo;

	__sys_freeaddrinfo  = freeaddrinfo;
	sys_freeaddrinfo    = &__sys_freeaddrinfo;

	__sys_gethostbyname = gethostbyname;
	sys_gethostbyname   = &__sys_gethostbyname;
#endif
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

void hook_once(void)
{
	if (pthread_once(&__once_control, hook_api) != 0) {
		abort();
	}
}
