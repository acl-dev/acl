#pragma once

#include <iostream>
#include <assert.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "acl_cpp/lib_acl.hpp"

extern acl::atomic_long __aio_refer;
extern int __success;
extern int __destroy;
extern int __disconnect;
extern int __ns_failed;
extern int __connect_ok;
extern int __connect_timeout;
extern int __connect_failed;
extern int __header_ok;
extern int __read_timeout;
