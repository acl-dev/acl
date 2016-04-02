#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_server.hpp"
#endif

namespace acl
{

#define INT_LEN		11

redis_server::redis_server()
: redis_command(NULL)
{
}

redis_server::redis_server(redis_client* conn)
: redis_command(conn)
{
}

redis_server::redis_server(redis_client_cluster* cluster, size_t max_conns)
: redis_command(cluster, max_conns)
{
}

redis_server::~redis_server()
{
}

bool redis_server::bgrewriteaof()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "BGREWRITEAOF";
	lens[0] = sizeof("BGREWRITEAOF") - 1;

	build_request(1, argv, lens);
	return check_status("Background");
}

bool redis_server::bgsave()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "BGSAVE";
	lens[0] = sizeof("BGSAVE") - 1;

	build_request(1, argv, lens);
	return check_status();
}

bool redis_server::client_getname(string& buf)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "CLIENT";
	lens[0] = sizeof("CLIENT") - 1;

	argv[1] = "GETNAME";
	lens[1] = sizeof("GETNAME") - 1;

	build_request(2, argv, lens);
	return get_string(buf) >0 ? true : false;
}

bool redis_server::client_kill(const char* addr)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "CLIENT";
	lens[0] = sizeof("CLIENT") - 1;

	argv[1] = "KILL";
	lens[1] = sizeof("KILL") - 1;

	argv[2] = addr;
	lens[2] = strlen(addr);

	build_request(3, argv, lens);
	return check_status();
}

int redis_server::client_list(string& buf)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "CLIENT";
	lens[0] = sizeof("CLIENT") - 1;

	argv[1] = "LIST";
	lens[1] = sizeof("LIST") - 1;

	build_request(2, argv, lens);
	return get_string(buf);
}

bool redis_server::client_setname(const char* name)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "CLIENT";
	lens[0] = sizeof("CLIENT") - 1;

	argv[1] = "SETNAME";
	lens[1] = sizeof("SETNAME") - 1;

	argv[2] = name;
	lens[2] = strlen(name);

	build_request(3, argv, lens);
	return check_status();
}

int redis_server::config_get(const char* parameter,
	std::map<string, string>& out)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "CONFIG";
	lens[0] = sizeof("CONFIG") - 1;

	argv[1] = "GET";
	lens[1] = sizeof("GET") - 1;

	argv[2] = parameter;
	lens[2] = strlen(parameter);

	build_request(3, argv, lens);
	return get_strings(out);
}

bool redis_server::config_resetstat()
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "CONFIG";
	lens[0] = sizeof("CONFIG") - 1;

	argv[1] = "RESETSTAT";
	lens[1] = sizeof("RESETSTAT") - 1;

	build_request(2, argv, lens);
	return check_status();
}

bool redis_server::config_rewrite()
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "CONFIG";
	lens[0] = sizeof("CONFIG") - 1;

	argv[1] = "REWRITE";
	lens[1] = sizeof("REWRITE") -1;

	build_request(2, argv, lens);
	return check_status();
}

bool redis_server::config_set(const char* name, const char* value)
{
	const char* argv[4];
	size_t lens[4];

	argv[0] = "CONFIG";
	lens[0] = sizeof("CONFIG") - 1;

	argv[1] = "SET";
	lens[1] = sizeof("SET") - 1;

	argv[2] = name;
	lens[2] = strlen(name);

	argv[3] = value;
	lens[3] = strlen(value);

	build_request(4, argv, lens);
	return check_status();
}

int redis_server::dbsize()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "DBSIZE";
	lens[0] = sizeof("DBSIZE") - 1;

	build_request(1, argv, lens);
	return get_number();
}

bool redis_server::flushall()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "FLUSHALL";
	lens[0] = sizeof("FLUSHALL") - 1;

	build_request(1, argv, lens);
	return check_status();
}

bool redis_server::flushdb()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "FLUSHDB";
	lens[0] = sizeof("FLUSHDB") - 1;

	build_request(1, argv, lens);
	return check_status();
}

int redis_server::info(string& buf)
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "INFO";
	lens[0] = sizeof("INFO") - 1;

	build_request(1, argv, lens);
	return get_string(buf);
}

int redis_server::info(std::map<string, string>& out)
{
	out.clear();
	string buf;
	int ret = info(buf);
	if (ret <= 0)
		return ret;

	string line;
	while (!buf.empty())
	{
		line.clear();
		buf.scan_line(line);
		if (line.empty())
			continue;

		char* name = line.c_str();
		if (*name == ':')
			name++;
		char* value = strchr(name, ':');
		if (value == NULL || *(value + 1) == 0)
			continue;
		*value++ = 0;
		out[name] = value;
	}

	return (int) out.size();
}

time_t redis_server::lastsave()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "LASTSAVE";
	lens[0] = sizeof("LASTSAVE") - 1;

	build_request(1, argv, lens);
	return (time_t) get_number64();
}

bool redis_server::monitor()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "MONITOR";
	lens[0] = sizeof("MONITOR") - 1;

	build_request(1, argv, lens);
	return check_status();
}

bool redis_server::get_command(string& buf)
{
	clear_request();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_STATUS)
		return false;
	const char* status = result->get_status();
	if (status == NULL || *status == '\0')
		return false;
	buf = status;
	return true;
}

bool redis_server::save()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "SAVE";
	lens[0] = sizeof("SAVE") - 1;

	build_request(1, argv, lens);
	return check_status();
}

void redis_server::shutdown(bool save_data /* = true */)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "SHUTDOWN";
	lens[0] = sizeof("SHUTDOWN") - 1;

	if (save_data)
	{
		argv[1] = "save";
		lens[1] = sizeof("save") - 1;
	}
	else
	{
		argv[1] = "nosave";
		lens[1] = sizeof("nosave") - 1;
	}

	build_request(2, argv, lens);
	(void) check_status();
}

bool redis_server::slaveof(const char* ip, int port)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "SLAVEOF";
	lens[0] = sizeof("SLAVEOF") - 1;

	argv[1] = ip;
	lens[1] = strlen(ip);

	char port_s[INT_LEN];
	safe_snprintf(port_s, sizeof(port_s), "%d", port);
	argv[2] = port_s;
	lens[2] = strlen(port_s);

	build_request(3, argv, lens);
	return check_status();
}

const redis_result* redis_server::slowlog_get(int number /* = 0 */)
{
	const char* argv[3];
	size_t lens[3];

	argv[0] = "SLOWLOG";
	lens[0] = sizeof("SLOWLOG") - 1;

	argv[1] = "GET";
	lens[1] = sizeof("GET") - 1;

	size_t argc = 2;

	char buf[INT_LEN];
	if (number > 0)
	{
		safe_snprintf(buf, sizeof(buf), "%d", number);
		argv[2] = buf;
		lens[2] = strlen(buf);
		argc++;
	}

	build_request(argc, argv, lens);
	return run();
}

int redis_server::slowlog_len()
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "SLOWLOG";
	lens[0] = sizeof("SLOWLOG") - 1;

	argv[1] = "LEN";
	lens[1] = sizeof("LEN") - 1;

	build_request(2, argv, lens);
	return get_number();
}

bool redis_server::slowlog_reset()
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "SLOWLOG";
	lens[0] = sizeof("SLOWLOG") - 1;

	argv[1] = "RESET";
	lens[1] = sizeof("RESET") - 1;

	build_request(2, argv, lens);
	return check_status();
}

bool redis_server::get_time(time_t& stamp, int& escape)
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "TIME";
	lens[0] = sizeof("TIME") - 1;

	build_request(1, argv, lens);

	std::vector<string> tokens;
	if (get_strings(tokens) <= 0 || tokens.size() < 2)
		return false;

	stamp = atol(tokens[0].c_str());
	escape = atoi(tokens[1].c_str());
	return true;
}

} // namespace acl
