#pragma once
#include "lib_acl.h"

void aio_client_start(ACL_AIO *aio, const char *addr, int max_connect);
void aio_client_init(void);
