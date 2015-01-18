#pragma once
#include "acl_cpp/acl_cpp_define.hpp"
#include <vector>
#include "acl_cpp/stream/socket_stream.hpp"
#include "acl_cpp/stdlib/string.hpp"
#include "acl_cpp/connpool/connect_client.hpp"

namespace acl
{

class dbuf_pool;
class redis_result;

class ACL_CPP_API redis_client : public connect_client
{
public:
	redis_client(const char* addr, int conn_timeout = 60,
		int rw_timeout = 30, bool retry = true);
	~redis_client();

	dbuf_pool* get_pool() const
	{
		return pool_;
	}

	void reset();

	void close();
	
	const redis_result* run(const string& request, size_t nobjs = 1);

	const string& build_request(size_t argc, const char* argv[],
		size_t argv_lens[], string* buf = NULL);

	const string& build_request(const std::vector<string>& args,
		string* buf = NULL);
	const string& build_request(const std::vector<const char*>& args,
		const std::vector<size_t>& lens, string* buf = NULL);

	/*******************************************************************/

	const string& build(const char* cmd, const char* key,
		const std::map<string, string>& attrs, string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const std::map<string, char*>& attrs, string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const std::map<string, const char*>& attrs, string* buf = NULL);

	const string& build(const char* cmd, const char* key,
		const std::map<int, string>& attrs, string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const std::map<int, char*>& attrs, string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const std::map<int, const char*>& attrs, string* buf = NULL);

	const string& build(const char* cmd, const char* key,
		const std::vector<string>& names,
		const std::vector<string>& values, string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const std::vector<char*>& names,
		const std::vector<char*>& values, string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const std::vector<const char*>& names,
		const std::vector<const char*>& values, string* buf = NULL);

	const string& build(const char* cmd, const char* key,
		const std::vector<int>& names,
		const std::vector<string>& values, string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const std::vector<int>& names,
		const std::vector<char*>& values, string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const std::vector<int>& names,
		const std::vector<const char*>& values, string* buf = NULL);

	const string& build(const char* cmd, const char* key,
		const char* names[], const char* values[], size_t argc,
		string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const int names[], const char* values[], size_t argc,
		string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const char* names[], const size_t names_len[],
		const char* values[], const size_t values_len[],
		size_t argc, string* buf = NULL);

	/*******************************************************************/

	const string& build(const char* cmd, const char* key,
		const std::vector<string>& names, string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const std::vector<char*>& names, string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const std::vector<const char*>& names, string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const std::vector<int>& names, string* buf = NULL);

	const string& build(const char* cmd, const char* key,
		const char* names[], size_t argc, string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const int names[], size_t argc, string* buf = NULL);
	const string& build(const char* cmd, const char* key,
		const char* names[], const size_t lens[],
		size_t argc, string* buf = NULL);

	/*******************************************************************/

protected:
	// »ùÀàÐéº¯Êý
	virtual bool open();

private:
	unsigned long long used_;
	dbuf_pool* pool_;
	socket_stream conn_;
	char* addr_;
	int   conn_timeout_;
	int   rw_timeout_;
	bool  retry_;
	size_t  argv_size_;
	const char**  argv_;
	size_t  argc_;
	size_t* argv_lens_;
	string  request_;
	string  buf_;
	redis_result* result_;

	void argv_space(size_t n);

	redis_result* get_objects(size_t nobjs);
	redis_result* get_object();
	redis_result* get_error();
	redis_result* get_status();
	redis_result* get_integer();
	redis_result* get_string();
	redis_result* get_array();
};

} // end namespace acl
