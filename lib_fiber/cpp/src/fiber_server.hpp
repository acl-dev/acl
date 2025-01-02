#pragma once

const char *acl_fiber_server_conf();
long long acl_fiber_server_users_count_add(int n);
long long acl_fiber_server_users_count();

void acl_fiber_server_main(int argc, char *argv[],
	void (*service)(void*, ACL_VSTREAM*), void *ctx, int name, ...);
