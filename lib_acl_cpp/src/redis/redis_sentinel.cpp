#include "acl_stdafx.hpp"
#include <map>
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_slave.hpp"
#include "acl_cpp/redis/redis_master.hpp"
#include "acl_cpp/redis/redis_sentinel.hpp"
#endif

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

redis_sentinel::redis_sentinel(void)
{
}

redis_sentinel::redis_sentinel(redis_client* conn)
: redis_command(conn)
{
}

redis_sentinel::~redis_sentinel(void)
{
}

//////////////////////////////////////////////////////////////////////////////

static void master_add_member(const char* name, const char* value,
	redis_master& out)
{
	if (strcasecmp(name, "name") == 0)
		out.name_ = value;
	else if (strcasecmp(name, "ip") == 0)
		out.ip_ = value;
	else if (strcasecmp(name, "port") == 0)
		out.port_ = atoi(value);
	else if (strcasecmp(name, "runid") == 0)
		out.runid_ = value;
	else if (strcasecmp(name, "flags") == 0)
		out.flags_ = value;
	else if (strcasecmp(name, "link-pending-commands") == 0)
		out.link_pending_commands_ = (unsigned) atoi(value);
	else if (strcasecmp(name, "link-refcount") == 0)
		out.link_refcount_ = (unsigned) atoi(value);
	else if (strcasecmp(name, "last-ping-sent") == 0)
		out.last_ping_sent_ = (unsigned) atoi(value);
	else if (strcasecmp(name, "last-ok-ping-reply") == 0)
		out.last_ok_ping_reply_ = (unsigned) atoi(value);
	else if (strcasecmp(name, "last-ping-reply") == 0)
		out.last_ping_reply_ = (unsigned) atoi(value);
	else if (strcasecmp(name, "down-after-milliseconds") == 0)
		out.down_after_milliseconds_ = (unsigned) atoi(value);
	else if (strcasecmp(name, "info-refresh") == 0)
		out.info_refresh_ = (unsigned) atoi(value);
	else if (strcasecmp(name, "role-reported") == 0)
		out.role_reported_ = value;
	else if (strcasecmp(name, "role-reported-time") == 0)
		out.role_reported_time_ = (time_t) atol(value);
	else if (strcasecmp(name, "config-epoch") == 0)
		out.config_epoch_ = (time_t) atol(value);
	else if (strcasecmp(name, "num-slaves") == 0)
		out.num_slaves_ = (unsigned) atoi(value);
	else if (strcasecmp(name, "num-other-sentinels") == 0)
		out.num_other_sentinels_ = (unsigned) atoi(value);
	else if (strcasecmp(name, "quorum") == 0)
		out.quorum_ = (unsigned) atoi(value);
	else if (strcasecmp(name, "failover-timeout") == 0)
		out.failover_timeout_ = (unsigned) atoi(value);
	else if (strcasecmp(name, "parallel-syncs") == 0)
		out.parallel_syncs_ = (unsigned) atoi(value);
}

bool redis_sentinel::sentinel_master(const char* name, redis_master& out)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "SENTINEL";
	lens[0] = sizeof("SENTINEL") - 1;

	argv[1] = "MASTER";
	lens[1] = sizeof("MASTER") - 1;

	argv[2] = name;
	lens[2] = strlen(argv[2]);

	build_request(3, argv, lens);

	std::map<string, string> result;
	if (get_strings(result) < 0)
		return false;

	for (std::map<string, string>::const_iterator cit = result.begin();
		cit != result.end(); ++cit) {

		master_add_member(cit->first, cit->second, out);
	}

	return true;
}

static void add_master(const redis_result& in, std::vector<redis_master>& out)
{
	if (in.get_type() != REDIS_RESULT_ARRAY)
		return;

	size_t size;
	const redis_result** children = in.get_children(&size);
	if (children == NULL)
		return;
	if (size % 2 != 0)
		return;

	redis_master master;

	string name, value;
	const redis_result* rr;
	for (size_t i = 0; i < size;) {
		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_STRING) {
			i += 2;
			continue;
		}
		name.clear();
		rr->argv_to_string(name);
		i++;

		rr = children[i];
		i++;
		if (rr->get_type() != REDIS_RESULT_STRING)
			continue;
		value.clear();
		rr->argv_to_string(value);
		master_add_member(name, value, master);
	}
	out.push_back(master);
}

bool redis_sentinel::sentinel_masters(std::vector<redis_master>& out)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "SENTINEL";
	lens[0] = sizeof("SENTINEL") - 1;

	argv[1] = "MASTERS";
	lens[1] = sizeof("MASTERS") - 1;

	build_request(2, argv, lens);
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return false;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return true;

	for (size_t i = 0; i < size; i++) {
		const redis_result* child = children[i];
		add_master(*child, out);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////

#define	EQ(x, y) !strcasecmp((x), (y))

static void slave_add_member(const char* name, const char* value,
	redis_slave& out)
{
	if (EQ(name, "name"))
		out.name_ = value;
	else if (EQ(name, "ip"))
		out.ip_ = value;
	else if (EQ(name, "port"))
		out.port_ = atoi(value);
	else if (EQ(name, "runid"))
		out.runid_ = value;
	else if (EQ(name, "flags"))
		out.flags_ = value;
	else if (EQ(name, "link-pending-commands"))
		out.link_pending_commands_ = (unsigned) atoi(value);
	else if (EQ(name, "link-refcount"))
		out.link_refcount_ = (unsigned) atoi(value);
	else if (EQ(name, "last-ping-sent"))
		out.last_ping_sent_ = (unsigned) atoi(value);
	else if (EQ(name, "last-ok-ping-reply"))
		out.last_ok_ping_reply_ = (unsigned) atoi(value);
	else if (EQ(name, "last-ping-reply"))
		out.last_ping_reply_ = (unsigned) atoi(value);
	else if (EQ(name, "down-after-milliseconds"))
		out.down_after_milliseconds_ = (unsigned) atoi(value);
	else if (EQ(name, "info-refresh"))
		out.info_refresh_ = (unsigned) atoi(value);
	else if (EQ(name, "role-reported"))
		out.role_reported_ = value;
	else if (EQ(name, "role-reported-time"))
		out.role_reported_time_ = (time_t) atol(value);
	else if (EQ(name, "master-link-down-time"))
		out.master_link_down_time_ = (time_t) atol(value);
	else if (EQ(name, "master-link-status"))
		out.master_link_status_ = value;
	else if (EQ(name, "master-host"))
		out.master_host_ = value;
	else if (EQ(name, "master-port"))
		out.master_port_ = atoi(value);
	else if (EQ(name, "slave-priority"))
		out.slave_priority_ = (unsigned) atoi(value);
	else if (EQ(name, "slave-repl-offset"))
		out.slave_repl_offset_ = (unsigned long) atol(value);
}

static void add_slave(const redis_result& in, std::vector<redis_slave>& out)
{
	if (in.get_type() != REDIS_RESULT_ARRAY)
		return;

	size_t size;
	const redis_result** children = in.get_children(&size);
	if (children == NULL)
		return;
	if (size % 2 != 0)
		return;

	redis_slave slave;
	string name, value;
	const redis_result* rr;
	for (size_t i = 0; i < size;) {
		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_STRING) {
			i += 2;
			continue;
		}
		name.clear();
		rr->argv_to_string(name);
		i++;

		rr = children[i];
		i++;
		if (rr->get_type() != REDIS_RESULT_STRING)
			continue;
		value.clear();
		rr->argv_to_string(value);
		slave_add_member(name, value, slave);
	}
	out.push_back(slave);
}

bool redis_sentinel::sentinel_slaves(const char* master_name,
	std::vector<redis_slave>& out)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "SENTINEL";
	lens[0] = sizeof("SENTINEL") - 1;

	argv[1] = "SLAVES";
	lens[1] = sizeof("SLAVES") - 1;

	argv[2] = master_name;
	lens[2] = strlen(master_name);

	build_request(3, argv, lens);
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY)
		return false;

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0)
		return true;

	for (size_t i = 0; i < size; i++) {
		const redis_result* child = children[i];
		add_slave(*child, out);
	}
	return true;
}

bool redis_sentinel::sentinel_get_master_addr_by_name(const char* master_name,
	string& ip, int& port)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "SENTINEL";
	lens[0] = sizeof("SENTINEL") - 1;

	argv[1] = "get-master-addr-by-name";
	lens[1] = sizeof("get-master-addr-by-name") - 1;

	argv[2] = master_name;
	lens[2] = strlen(master_name);

	build_request(3, argv, lens);

	port = -1;
	std::vector<string> result;
	if (get_strings(&result) != 2)
		return false;

	ip = result[0];
	port = atoi(result[1]);
	return true;
}

int redis_sentinel::sentinel_reset(const char* pattern)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "SENTINEL";
	lens[0] = sizeof("SENTINEL") - 1;

	argv[1] = "reset";
	lens[0] = sizeof("reset") - 1;

	argv[2] = pattern;
	lens[2] = strlen(pattern);

	build_request(3, argv, lens);
	return get_number();
}

bool redis_sentinel::sentinel_failover(const char* master_name)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "SENTINEL";
	lens[0] = sizeof("SENTINEL") - 1;

	argv[1] = "failover";
	lens[1] = sizeof("failover") - 1;

	argv[2] = master_name;
	lens[2] = strlen(master_name);

	build_request(3, argv, lens);
	return check_status();
}

bool redis_sentinel::sentinel_flushconfig(void)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "SENTINEL";
	lens[0] = sizeof("SENTINEL") - 1;

	argv[1] = "flushconfig";
	lens[1] = sizeof("flushconfig") - 1;

	build_request(2, argv, lens);
	return check_status();
}

bool redis_sentinel::sentinel_remove(const char* master_name)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "SENTINEL";
	lens[0] = sizeof("SENTINEL") - 1;

	argv[1] = "remove";
	lens[1] = sizeof("remove") - 1;

	argv[2] = master_name;
	lens[2] = strlen(master_name);

	build_request(3, argv, lens);
	return check_status();
}

bool redis_sentinel::sentinel_monitor(const char* master_name, const char* ip,
	int port, int quorum)
{
	const char* argv[6];
	size_t lens[6];

	argv[0] = "SENTINEL";
	lens[0] = sizeof("SENTINEL") - 1;

	argv[1] = "monitor";
	lens[1] = sizeof("monitor") - 1;

	argv[2] = master_name;
	lens[2] = strlen(master_name);

	argv[3] = ip;
	lens[3] = strlen(ip);

	char port_s[64];
	safe_snprintf(port_s, sizeof(port_s), "%d", port);
	argv[4] = port_s;
	lens[4] = strlen(argv[4]);

	char quorum_s[64];
	safe_snprintf(quorum_s, sizeof(quorum_s), "%d", quorum);
	argv[5] = quorum_s;
	lens[5] = strlen(argv[5]);

	build_request(6, argv, lens);
	return check_status();
}

bool redis_sentinel::sentinel_set(const char* master_name, const char* name,
	const char* value)
{
	const char* argv[5];
	size_t lens[5];

	argv[0] = "SENTINEL";
	lens[0] = sizeof("SENTINEL") - 1;

	argv[1] = "SET";
	lens[1] = sizeof("SET") - 1;

	argv[2] = master_name;
	lens[2] = strlen(argv[2]);

	argv[3] = name;
	lens[3] = strlen(argv[3]);

	argv[4] = value;
	lens[4] = strlen(argv[4]);

	build_request(5, argv, lens);
	return check_status();
}

bool redis_sentinel::sentinel_set(const char* master_name, const char* name,
	unsigned value)
{
	char buf[64];
	safe_snprintf(buf, sizeof(buf), "%u", value);
	return sentinel_set(master_name, name, buf);
}

}

#endif // ACL_CLIENT_ONLY
