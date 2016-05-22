#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/redis/redis_command.hpp"

namespace acl
{

class redis_client;
class redis_result;

class ACL_CPP_API redis_script : virtual public redis_command
{
public:
	/**
	 * see redis_command::redis_command()
	 */
	redis_script(void);

	/**
	 * see redis_command::redis_command(redis_client*)
	 */
	redis_script(redis_client* conn);

	/**
	 * see redis_command::redis_command(redis_client_cluster*, size_t)
	 */
	redis_script(redis_client_cluster* cluster, size_t max_conns = 0);

	virtual ~redis_script(void);

	/////////////////////////////////////////////////////////////////////

	const redis_result* eval(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args);
	const redis_result* eval(const char* script,
		const std::vector<const char*>& keys,
		const std::vector<const char*>& args);

	const redis_result* evalsha(const char* sha1,
		const std::vector<string>& keys,
		const std::vector<string>& args);
	const redis_result* evalsha(const char* sha1,
		const std::vector<const char*>& keys,
		const std::vector<const char*>& args);

	int script_exists(const std::vector<string>& scripts,
		std::vector<bool>& out);
	int script_exists(const std::vector<const char*>& scripts,
		std::vector<bool>& out);

	bool script_flush();
	bool script_load(const string& script, string& out);
	bool script_kill();

	/////////////////////////////////////////////////////////////////////

	bool eval_status(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		const char* success = "OK");
	bool eval_number(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		int& out);
	bool eval_number64(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		long long int& out);
	int eval_string(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		string& out);

	bool evalsha_status(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		const char* success = "OK");
	bool evalsha_number(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		int& out);
	bool evalsha_number64(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		long long int& out);
	int evalsha_string(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		string& out);

	int eval_status(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		std::vector<bool>& out,
		const char* success = "OK");
	int eval_number(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		std::vector<int>& out,
		std::vector<bool>& status);
	long long int eval_number64(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		std::vector<long long int>& out,
		std::vector<bool>& status);
	int eval_strings(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		std::vector<string>& out);

	int evalsha_status(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		std::vector<bool>& out,
		const char* success = "OK");
	int evalsha_number(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		std::vector<int>& out,
		std::vector<bool>& status);
	long long int evalsha_number64(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		std::vector<long long int>& out,
		std::vector<bool>& status);
	int evalsha_strings(const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		std::vector<string>& out);

private:
	int eval_status(const char* cmd, const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		std::vector<bool>& out,
		const char* success = "OK");
	int eval_number(const char* cmd, const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		std::vector<int>& out,
		std::vector<bool>& status);
	long long int eval_number64(const char* cmd, const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		std::vector<long long int>& out,
		std::vector<bool>& status);
	int eval_strings(const char* cmd, const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args,
		std::vector<string>& out);

	const redis_result* eval_cmd(const char* cmd, const char* script,
		const std::vector<string>& keys,
		const std::vector<string>& args);
	const redis_result* eval_cmd(const char* cmd, const char* script,
		const std::vector<const char*>& keys,
		const std::vector<const char*>& args);
};

} // namespace acl
