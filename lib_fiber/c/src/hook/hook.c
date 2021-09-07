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

setsockopt_fn __sys_setsockopt              = NULL;
setsockopt_fn *sys_setsockopt               = NULL;

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

# ifdef __USE_LARGEFILE64
sendfile64_fn __sys_sendfile64              = NULL;
sendfile64_fn *sys_sendfile64               = NULL;
# endif

# ifdef HAS_EPOLL
epoll_create_fn __sys_epoll_create          = NULL;
epoll_create_fn *sys_epoll_create           = NULL;

epoll_wait_fn   __sys_epoll_wait            = NULL;
epoll_wait_fn   *sys_epoll_wait             = NULL;

epoll_ctl_fn    __sys_epoll_ctl             = NULL;
epoll_ctl_fn    *sys_epoll_ctl              = NULL;
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

#define LOAD_FN(name, type, fn, fp) do { \
	(fn) = (type) dlsym(RTLD_NEXT, name); \
	assert((fn)); \
	(fp) = &(fn); \
} while (0)

	LOAD_FN("socket", socket_fn, __sys_socket, sys_socket);
	LOAD_FN("close", close_fn, __sys_close, sys_close);
	LOAD_FN("listen", listen_fn, __sys_listen, sys_listen);
	LOAD_FN("accept", accept_fn, __sys_accept, sys_accept);
	LOAD_FN("connect", connect_fn, __sys_connect, sys_connect);
	LOAD_FN("setsockopt", setsockopt_fn, __sys_setsockopt, sys_setsockopt);
	LOAD_FN("sleep", sleep_fn, __sys_sleep, sys_sleep);
	LOAD_FN("read", read_fn, __sys_read, sys_read);
	LOAD_FN("readv", readv_fn, __sys_readv, sys_readv);
	LOAD_FN("recv", recv_fn, __sys_recv, sys_recv);
	LOAD_FN("recvfrom", recvfrom_fn, __sys_recvfrom, sys_recvfrom);
	LOAD_FN("recvmsg", recvmsg_fn, __sys_recvmsg, sys_recvmsg);
	LOAD_FN("write", write_fn, __sys_write, sys_write);
	LOAD_FN("writev", writev_fn, __sys_writev, sys_writev);
	LOAD_FN("send", send_fn, __sys_send, sys_send);
	LOAD_FN("sendto", sendto_fn, __sys_sendto, sys_sendto);
	LOAD_FN("sendmsg", sendmsg_fn, __sys_sendmsg, sys_sendmsg);

# ifdef __USE_LARGEFILE64
	LOAD_FN("sendfile64", sendfile64_fn, __sys_sendfile64, sys_sendfile64);
# endif
	LOAD_FN("poll", poll_fn, __sys_poll, sys_poll);
	LOAD_FN("select", select_fn, __sys_select, sys_select);

# ifdef	HAS_EPOLL
	LOAD_FN("epoll_create", epoll_create_fn, __sys_epoll_create, sys_epoll_create);

	LOAD_FN("epoll_wait", epoll_wait_fn, __sys_epoll_wait, sys_epoll_wait);

	LOAD_FN("epoll_ctl", epoll_ctl_fn, __sys_epoll_ctl, sys_epoll_ctl);
# endif // HAS_EPOLL

	LOAD_FN("getaddrinfo", getaddrinfo_fn, __sys_getaddrinfo, sys_getaddrinfo);
	LOAD_FN("freeaddrinfo", freeaddrinfo_fn, __sys_freeaddrinfo, sys_freeaddrinfo);
	LOAD_FN("gethostbyname", gethostbyname_fn, __sys_gethostbyname, sys_gethostbyname);

# ifndef __APPLE__
	LOAD_FN("gethostbyname_r", gethostbyname_r_fn, __sys_gethostbyname_r, sys_gethostbyname_r);
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
