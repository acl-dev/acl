#ifndef __WIN_PATCH_INCLUDE_H__
#define __WIN_PATCH_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
# include <winsock2.h>
# define HAVE_NO_GETOPT
# define snprintf _snprintf
/*
extern int   optind;
extern char *optarg;
int getopt(int argc, char *argv[], const char *opts);
*/
#else
# define SOCKET		int
# define INVALID_SOCKET -1
#endif

void socket_init(void);
void socket_end(void);
void socket_close(SOCKET fd);
SOCKET socket_listen(const char *ip, int port);
SOCKET socket_accept(SOCKET fd);
SOCKET socket_connect(const char *ip, int port);
int set_non_blocking(SOCKET fd, int on);
int socket_is_non_blocking(SOCKET fd);

#ifdef __cplusplus
}
#endif

#endif
