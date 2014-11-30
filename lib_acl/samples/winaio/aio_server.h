#pragma once
#include "lib_acl.h"

void aio_server_log_fn(void (*write_fn)(void *, const char *fmt, ...),
			void *write_arg,
			void (*fflush_fn)(void *),
			void *fflush_arg);
void aiho_server_start(ACL_AIO *aio, const char *addr, int accept_auto, int echo_src);
void aio_server_end(void);

