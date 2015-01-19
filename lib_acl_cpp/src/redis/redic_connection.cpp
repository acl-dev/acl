#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_connection.hpp"

namespace acl
{

redis_connection::redis_connection(redis_client* conn /* = NULL */)
: redis_command(conn)
{

}

redis_connection::~redis_connection()
{

}

bool redis_connection::auth(const char* passwd)
{
	const char* argv[2];
	size_t lens[2];

	argv[0] = "AUTH";
	lens[0] = strlen(argv[0]);

	argv[1] = passwd;
	lens[1] = strlen(argv[1]);

	const string& req = conn_->build_request(2, argv, lens);
	return conn_->get_status(req);
}

bool redis_connection::select(int dbnum)
{
	const char* argv[2];
	size_t lens[2];
	
	argv[0] = "SELECT";
	lens[0] = strlen(argv[0]);

	char buf[21];
	(void) safe_snprintf(buf, sizeof(buf), "%d", dbnum);
	argv[1] = buf;
	lens[1] = strlen(argv[1]);

	const string& req = conn_->build_request(2, argv, lens);
	return conn_->get_status(req);
}

bool redis_connection::ping()
{
	const char* argv[1];
	size_t lens[1];
	
	argv[0] = "PING";
	lens[0] = strlen(argv[0]);

	const string& req = conn_->build_request(1, argv, lens);
	return conn_->get_status(req, "PONG");
}

bool redis_connection::echo(const char* s)
{
	const char* argv[2];
	size_t lens[2];
	
	argv[0] = "ECHO";
	lens[0] = strlen(argv[0]);

	argv[1] = s;
	lens[1] = strlen(argv[1]);

	string buf;
	const string& req = conn_->build_request(2, argv, lens);
	return conn_->get_string(req, buf) >= 0 ? true : false;
}

bool redis_connection::quit()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "QUIT";
	lens[0] = strlen(argv[0]);

	const string& req = conn_->build_request(1, argv, lens);
	bool ret = conn_->get_status(req);
	conn_->close();
	return ret;
}

} // namespace acl
