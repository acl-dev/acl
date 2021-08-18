#ifndef __HOOK_HEAD_H__
#define __HOOK_HEAD_H__

#include "fiber/fiber_define.h"

//extern struct dns_resolv_conf *var_dns_conf;
//extern struct dns_hosts *var_dns_hosts;
//extern struct dns_hints *var_dns_hints;

extern void fiber_dns_set_read_wait(int timeout);
extern void fiber_dns_init(void);

typedef socket_t (WINAPI *socket_fn)(int, int, int);
typedef int (WINAPI *close_fn)(socket_t);
typedef int (WINAPI *listen_fn)(socket_t, int);
typedef socket_t (WINAPI *accept_fn)(socket_t, struct sockaddr *, socklen_t *);
typedef int (WINAPI *connect_fn)(socket_t, const struct sockaddr *, socklen_t);

#if defined(SYS_WIN)

typedef int (WINAPI *recv_fn)(socket_t, char *, int, int);
typedef int (WINAPI *recvfrom_fn)(socket_t, char *, int, int,
	struct sockaddr *, socklen_t *);
typedef int (WINAPI *send_fn)(socket_t, const char *, int, int);
typedef int (WINAPI *sendto_fn)(socket_t, const char *, int, int,
	const struct sockaddr *, socklen_t);
typedef int (WINAPI *poll_fn)(struct pollfd *, nfds_t, int);
typedef int (WINAPI *select_fn)(int, fd_set *, fd_set *,
	fd_set *, const struct timeval *);

#elif defined(SYS_UNIX)

typedef int (*setsockopt_fn)(socket_t, int, int, const void *, socklen_t);
typedef unsigned (*sleep_fn)(unsigned int seconds);
typedef ssize_t  (*read_fn)(socket_t, void *, size_t);
typedef ssize_t  (*readv_fn)(socket_t, const struct iovec *, int);
typedef ssize_t  (*recv_fn)(socket_t, void *, size_t, int);
typedef ssize_t  (*recvfrom_fn)(socket_t, void *, size_t, int,
typedef ssize_t  (*recvmsg_fn)(socket_t, struct msghdr *, int);
typedef ssize_t  (*write_fn)(socket_t, const void *, size_t);
typedef ssize_t  (*writev_fn)(socket_t, const struct iovec *, int);
typedef ssize_t  (*send_fn)(socket_t, const void *, size_t, int);
typedef ssize_t  (*sendto_fn)(socket_t, const void *, size_t, int,
	const struct sockaddr *, socklen_t);
typedef ssize_t  (*sendmsg_fn)(socket_t, const struct msghdr *, int);

# ifdef  __USE_LARGEFILE64
typedef ssize_t  (*sendfile64_fn)(socket_t, int, off64_t*, size_t);
# endif

typedef int (*poll_fn)(struct pollfd *, nfds_t, int);
typedef int (*select_fn)(int, fd_set *, fd_set *, fd_set *, struct timeval *);

# ifdef	HAS_EPOLL
typedef int (*epoll_create_fn)(int);
typedef int (*epoll_wait_fn)(int, struct epoll_event *,int, int);
typedef int (*epoll_ctl_fn)(int, int, int, struct epoll_event *);
# endif

typedef int (*getaddrinfo_fn)(const char *node, const char *service,
	const struct addrinfo* hints, struct addrinfo **res);
typedef void (*freeaddrinfo_fn)(struct addrinfo *res);
typedef struct hostent *(*gethostbyname_fn)(const char *);

# ifndef __APPLE__
typedef int (*gethostbyname_r_fn)(const char *, struct hostent *, char *,
	size_t, struct hostent **, int *);
# endif

#endif

extern socket_fn            __sys_socket;
extern close_fn             __sys_close;
extern listen_fn            __sys_listen;
extern accept_fn            __sys_accept;
extern connect_fn           __sys_connect;

extern recv_fn              __sys_recv;
extern recvfrom_fn          __sys_recvfrom;

extern send_fn              __sys_send;
extern sendto_fn            __sys_sendto;
extern poll_fn              __sys_poll;
extern select_fn            __sys_select;

#if defined(SYS_UNIX)

extern sleep_fn             __sys_sleep;
extern setsockopt_fn        __sys_setsockopt;

extern read_fn              __sys_read;
extern readv_fn             __sys_readv;
extern recvmsg_fn           __sys_recvmsg;

extern write_fn             __sys_write;
extern writev_fn            __sys_writev;
extern sendmsg_fn           __sys_sendmsg;

# ifdef __USE_LARGEFILE64
extern sendfile64_fn        __sys_sendfile64;
# endif

# ifdef	HAS_EPOLL
extern epoll_create_fn      __sys_epoll_create;
extern epoll_wait_fn        __sys_epoll_wait;
extern epoll_ctl_fn         __sys_epoll_ctl;
# endif

extern getaddrinfo_fn       __sys_getaddrinfo;
extern freeaddrinfo_fn      __sys_freeaddrinfo;
extern gethostbyname_fn     __sys_gethostbyname;

# ifndef __APPLE__
extern gethostbyname_r_fn   __sys_gethostbyname_r;
# endif

#endif // SYS_UNIX

void hook_once(void);

#endif
