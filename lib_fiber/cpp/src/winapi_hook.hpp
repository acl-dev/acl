#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_WIN32) && !defined(_WIN64)
# define WINAPI
#endif

typedef socket_t (WINAPI* SOCKET_FN)(int, int, int);
typedef int (WINAPI* LISTEN_FN)(socket_t, int);
typedef int (WINAPI* CLOSE_FN)(socket_t);
typedef socket_t (WINAPI* ACCEPT_FN)(socket_t, struct sockaddr*, socklen_t*);
typedef int (WINAPI* CONNECT_FN)(socket_t, const struct sockaddr*, socklen_t);
typedef int (WINAPI* RECV_FN)(socket_t, char*, int, int);
typedef int (WINAPI* RECVFROM_FN)(socket_t, char*, int, int, struct sockaddr*, socklen_t*);
typedef int (WINAPI* SEND_FN)(socket_t, const char*, int, int);
typedef int (WINAPI* SENDTO_FN)(socket_t, const char*, int, int, const struct sockaddr*, socklen_t);
typedef int (WINAPI* SELECT_FN)(int, fd_set*, fd_set*, fd_set*, const struct timeval*);
typedef int (WINAPI* POLL_FN)(struct pollfd*, unsigned long, int);

extern SOCKET_FN    __socket;
extern LISTEN_FN    __listen;
extern CLOSE_FN     __close;
extern ACCEPT_FN    __accept;
extern CONNECT_FN   __connect;
extern RECV_FN      __recv;
extern RECVFROM_FN  __recvfrom;
extern SEND_FN      __send;
extern SENDTO_FN    __sendto;
extern SELECT_FN    __select;
extern POLL_FN      __poll;

bool winapi_hook(void);

#ifdef __cplusplus
}
#endif

