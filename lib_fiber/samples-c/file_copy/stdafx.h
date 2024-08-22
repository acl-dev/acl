#ifndef __STDAFX_INCLUDE_H__
#define __STDAFX_INCLUDE_H__

#define	_GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#ifdef	__APPLE__
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#elif	defined(__linux__)
#include <sys/sendfile.h>
#endif
#include "lib_acl.h"
#include "fiber/libfiber.h"

#endif
