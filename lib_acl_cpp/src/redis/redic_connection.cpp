#include "acl_stdafx.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_connection.hpp"

namespace acl
{

redis_connection::redis_connection(redis_client* conn /* = NULL */)
: conn_(conn)
{

}

redis_connection::~redis_connection()
{

}

void redis_connection::set_client(redis_client* conn)
{
	conn_ = conn;
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
	result_ = conn_->run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_STATUS)
		return false;
	const char* res = result_->get(0);
	if (res == NULL || strcasecmp(res, "OK") != 0)
		return false;
	else
		return true;
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
	result_ = conn_->run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_STATUS)
		return false;
	const char* res = result_->get(0);
	if (res == NULL || strcasecmp(res, "ok") != 0)
		return false;
	return true;
}

bool redis_connection::ping()
{
	const char* argv[1];
	size_t lens[1];
	
	argv[0] = "PING";
	lens[0] = strlen(argv[0]);

	const string& req = conn_->build_request(1, argv, lens);
	result_ = conn_->run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_STATUS)
		return false;
	const char* res = result_->get(0);
	if (res == NULL || strcasecmp(res, "PONG") != 0)
		return false;
	return true;
}

bool redis_connection::echo(const char* s)
{
	const char* argv[2];
	size_t lens[2];
	
	argv[0] = "ECHO";
	lens[0] = strlen(argv[0]);

	argv[1] = s;
	lens[1] = strlen(argv[1]);

	const string& req = conn_->build_request(2, argv, lens);
	result_  = conn_->run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_STRING)
		return false;
	string buf;
	result_->argv_to_string(buf);
	if (buf != s)
		return false;
	else
		return true;
}

bool redis_connection::quit()
{
	const char* argv[1];
	size_t lens[1];

	argv[0] = "QUIT";
	lens[0] = strlen(argv[0]);

	const string& req = conn_->build_request(1, argv, lens);
	result_ = conn_->run(req);
	if (result_ == NULL)
		return false;
	if (result_->get_type() != REDIS_RESULT_STATUS)
		return false;
	const char* res = result_->get(0);
	if (res == NULL || strcasecmp(res, "OK") != 0)
		return false;

	conn_->close();
	return true;
}

} // namespace acl
