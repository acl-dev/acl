#include "stdafx.h"
#include "common.h"
#include "hook.h"

socket_fn     __sys_socket                  = NULL;
listen_fn     __sys_listen                  = NULL;
accept_fn     __sys_accept                  = NULL;
connect_fn    __sys_connect                 = NULL;
close_fn      __sys_close                   = NULL;

recv_fn       __sys_recv                    = NULL;
recvfrom_fn   __sys_recvfrom                = NULL;

send_fn       __sys_send                    = NULL;
sendto_fn     __sys_sendto                  = NULL;
poll_fn       __sys_poll                    = NULL;
select_fn     __sys_select                  = NULL;

#ifdef SYS_UNIX

sleep_fn    __sys_sleep                     = NULL;
setsockopt_fn __sys_setsockopt              = NULL;

read_fn     __sys_read                      = NULL;
readv_fn    __sys_readv                     = NULL;
recvmsg_fn  __sys_recvmsg                   = NULL;

write_fn    __sys_write                     = NULL;
writev_fn   __sys_writev                    = NULL;
sendmsg_fn  __sys_sendmsg                   = NULL;

# ifdef __USE_LARGEFILE64
sendfile64_fn __sys_sendfile64              = NULL;
# endif

# ifdef HAS_EPOLL
epoll_create_fn __sys_epoll_create          = NULL;
epoll_wait_fn   __sys_epoll_wait            = NULL;
epoll_ctl_fn    __sys_epoll_ctl             = NULL;
# endif

getaddrinfo_fn   __sys_getaddrinfo          = NULL;
freeaddrinfo_fn  __sys_freeaddrinfo         = NULL;
gethostbyname_fn __sys_gethostbyname        = NULL;

# ifndef __APPLE__
gethostbyname_r_fn __sys_gethostbyname_r    = NULL;
# endif

#endif // SYS_UNIX

static void hook_api(void)
{
#ifdef SYS_UNIX
	__sys_socket     = (socket_fn) dlsym(RTLD_NEXT, "socket");
	assert(__sys_socket);

	__sys_close      = (close_fn) dlsym(RTLD_NEXT, "close");
	assert(__sys_close);

	__sys_listen     = (listen_fn) dlsym(RTLD_NEXT, "listen");
	assert(__sys_listen);

	__sys_accept     = (accept_fn) dlsym(RTLD_NEXT, "accept");
	assert(__sys_accept);

	__sys_connect    = (connect_fn) dlsym(RTLD_NEXT, "connect");
	assert(__sys_connect);

	__sys_setsockopt = (setsockopt_fn) dlsym(RTLD_NEXT, "setsockopt");
	assert(__sys_setsockopt);

	__sys_sleep      = (sleep_fn) dlsym(RTLD_NEXT, "sleep");
	assert(__sys_sleep);

	__sys_read       = (read_fn) dlsym(RTLD_NEXT, "read");
	assert(__sys_read);

	__sys_readv      = (readv_fn) dlsym(RTLD_NEXT, "readv");
	assert(__sys_readv);

	__sys_recv       = (recv_fn) dlsym(RTLD_NEXT, "recv");
	assert(__sys_recv);

	__sys_recvfrom   = (recvfrom_fn) dlsym(RTLD_NEXT, "recvfrom");
	assert(__sys_recvfrom);

	__sys_recvmsg    = (recvmsg_fn) dlsym(RTLD_NEXT, "recvmsg");
	assert(__sys_recvmsg);

	__sys_write      = (write_fn) dlsym(RTLD_NEXT, "write");
	assert(__sys_write);

	__sys_writev     = (writev_fn) dlsym(RTLD_NEXT, "writev");
	assert(__sys_writev);

	__sys_send       = (send_fn) dlsym(RTLD_NEXT, "send");
	assert(__sys_send);

	__sys_sendto     = (sendto_fn) dlsym(RTLD_NEXT, "sendto");
	assert(__sys_sendto);

	__sys_sendmsg    = (sendmsg_fn) dlsym(RTLD_NEXT, "sendmsg");
	assert(__sys_sendmsg);

# ifdef __USE_LARGEFILE64
	__sys_sendfile64 = (sendfile64_fn) dlsym(RTLD_NEXT, "sendfile64");
	assert(__sys_sendfile64);
# endif
	__sys_poll       = (poll_fn) dlsym(RTLD_NEXT, "poll");
	assert(__sys_poll);
	__sys_select = (select_fn) dlsym(RTLD_NEXT, "select");
	assert(__sys_select);

# ifdef	HAS_EPOLL
	__sys_epoll_create = (epoll_create_fn) dlsym(RTLD_NEXT, "epoll_create");
	assert(__sys_epoll_create);

	__sys_epoll_wait   = (epoll_wait_fn) dlsym(RTLD_NEXT, "epoll_wait");
	assert(__sys_epoll_wait);

	__sys_epoll_ctl    = (epoll_ctl_fn) dlsym(RTLD_NEXT, "epoll_ctl");
	assert(__sys_epoll_ctl);
# endif // HAS_EPOLL

	__sys_getaddrinfo = (getaddrinfo_fn) dlsym(RTLD_NEXT, "getaddrinfo");
	assert(__sys_getaddrinfo);

	__sys_freeaddrinfo = (freeaddrinfo_fn) dlsym(RTLD_NEXT, "freeaddrinfo");
	assert(__sys_freeaddrinfo);

	__sys_gethostbyname = (gethostbyname_fn) dlsym(RTLD_NEXT,
			"gethostbyname");
	assert(__sys_gethostbyname);

# ifndef __APPLE__
	__sys_gethostbyname_r = (gethostbyname_r_fn) dlsym(RTLD_NEXT,
			"gethostbyname_r");
	assert(__sys_gethostbyname_r);
# endif
#elif defined(SYS_WIN)
	__sys_socket   = socket;
	__sys_listen   = listen;
	__sys_accept   = accept;
	__sys_connect  = connect;
	__sys_close    = closesocket;
	__sys_recv     = (recv_fn) recv;
	__sys_recvfrom = (recvfrom_fn) recvfrom;
	__sys_send     = (send_fn) send;
	__sys_sendto   = (sendto_fn) sendto;
	__sys_poll     = WSAPoll;
	__sys_select   = select;
#endif
}

static pthread_once_t __once_control = PTHREAD_ONCE_INIT;

void hook_once(void)
{
	if (pthread_once(&__once_control, hook_api) != 0) {
		abort();
	}
}

