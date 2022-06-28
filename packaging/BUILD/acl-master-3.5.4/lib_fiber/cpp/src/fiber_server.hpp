#pragma once

const char *acl_fiber_server_conf(void);
void acl_fiber_server_main(int argc, char *argv[],
	void (*service)(void*, ACL_VSTREAM*), void *ctx, int name, ...);
