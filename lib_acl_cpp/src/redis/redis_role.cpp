#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_role.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

redis_role::redis_role(void)
{
}

redis_role::redis_role(redis_client* conn)
: redis_command(conn)
{
}

bool redis_role::role(void)
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "ROLE";
	lens[0] = sizeof("ROLE") - 1;

	build_request(1, argv, lens);
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY) {
		return false;
	}

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0) {
		return false;
	}

	const redis_result* first = children[0];
	first->argv_to_string(role_name_);

	if (role_name_.equal("sentinel", false)) {
		return role_sentinel(children, size);
	} else if (role_name_.equal("master", false)) {
		return role_master(children, size);
	} else if (role_name_.equal("slave", false)) {
		return role_slave(children, size);
	} else {
		logger_error("unknown role name=%s", role_name_.c_str());
		return false;
	}

	return true;
}

bool redis_role::role_sentinel(const redis_result** a, size_t n)
{
	string name;
	for (size_t i = 1; i < n; i++) {
		name.clear();
		a[i]->argv_to_string(name);
		masters_.push_back(name);
	}
	return true;
}

bool redis_role::role_master(const redis_result** a, size_t n)
{
	if (n < 3) {
		logger_error("tool small, n=%ld", (long) n);
		return false;
	}

	const redis_result* rr = a[1];
	bool ok;
	long long off = rr->get_integer64(&ok);
	if (!ok) {
		logger_error("can't get offset");
		return false;
	}
	role4master_.set_offset(off);

	rr = a[2];
	const redis_result** aa = rr->get_children(&n);
	for (size_t i = 0; i < n; i++) {
		if (add_one_slave(aa[i], role4master_) == false) {
			return false;
		}
	}

	return true;
}

bool redis_role::add_one_slave(const redis_result* a, redis_role4master& out)
{
	string buf;
	size_t size;
	const redis_result** children = a->get_children(&size);
	if (size < 3) {
		logger_error("invalid size=%d", (int) size);
		return false;
	}

	redis_role4slave slave;

	if (children[0]->get_type() != REDIS_RESULT_STRING) {
		logger_error("no ip");
		return false;
	}
	children[0]->argv_to_string(buf);
	slave.set_ip(buf);

	if (children[1]->get_type() != REDIS_RESULT_STRING) {
		logger_error("no port");
		return false;
	}
	buf.clear();
	children[1]->argv_to_string(buf);
	slave.set_port(atoi(buf.c_str()));

	if (children[2]->get_type() != REDIS_RESULT_STRING) {
		logger_error("no offset");
		return false;
	}
	buf.clear();
	children[2]->argv_to_string(buf);
	slave.set_offset(acl_atoi64(buf.c_str()));

	out.add_slave(slave);
	return true;
}

bool redis_role::role_slave(const redis_result** a, size_t n)
{
	if (n < 5) {
		logger_error("redis_result's size(%d) too small", (int) n);
		return false;
	}

	if (a[1]->get_type() != REDIS_RESULT_STRING) {
		logger_error("no ip");
		return false;
	}
	string buf;
	a[1]->argv_to_string(buf);
	role4slave_.set_ip(buf);

	if (a[2]->get_type() != REDIS_RESULT_INTEGER) {
		logger_error("no port");
		return false;
	}
	int port = a[2]->get_integer();
	role4slave_.set_port(port);

	if (a[3]->get_type() != REDIS_RESULT_STRING) {
		logger_error("no status");
		return false;
	}
	buf.clear();
	a[3]->argv_to_string(buf);
	role4slave_.set_status(buf);

	if (a[4]->get_type() != REDIS_RESULT_INTEGER) {
		logger_error("no offset");
		return false;
	}
	long long off = a[4]->get_integer64();
	role4slave_.set_offset(off);

	return true;
}

}

#endif // ACL_CLIENT_ONLY
