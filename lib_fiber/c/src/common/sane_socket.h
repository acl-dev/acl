#ifndef SANE_SOCKET_INCLUDE_H
#define SANE_SOCKET_INCLUDE_H

#include "fiber/libfiber.h"

int is_listen_socket(socket_t fd);

// return: -1, AF_INET¡¢AF_INET6 »ò AF_UNIX
int getsockfamily(socket_t fd);

// return: -1, SOCK_STREAM, SOCK_DGRAM
int getsocktype(socket_t fd);

#endif
