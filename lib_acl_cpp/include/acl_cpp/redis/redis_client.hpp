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
class redis_request;

class ACL_CPP_API redis_client : public connect_client
{
public:
	redis_client(const char* addr, int conn_timeout = 60,
		int rw_timeout = 30, bool retry = true);
	~redis_client();

	void set_slice_request(bool on);

	dbuf_pool* get_pool() const
	{
		return pool_;
	}

	socket_stream* get_stream();

	void reset();
	void close();
	
	void reset_request();

	const redis_result* get_result() const
	{
		return result_;
	}

	const redis_result* run(size_t nchildren = 0);

	int get_number(bool* success = NULL);
	long long int get_number64(bool* success = NULL);
	int get_number(std::vector<int>& out);
	int get_number64(std::vector<long long int>& out);
	bool get_status(const char* success = "OK");

	int get_status(std::vector<bool>& out);
	const char* get_status_string();

	int get_string(string& buf);
	int get_string(string* buf);
	int get_string(char* buf, size_t size);
	int get_strings(std::vector<string>& result);
	int get_strings(std::vector<string>* result);
	int get_strings(std::map<string, string>& result);
	int get_strings(std::vector<string>& names, std::vector<string>& values);
	int get_strings(std::vector<const char*>& names,
		std::vector<const char*>& values);

	const char* get_value(size_t i, size_t* len = NULL);
	const redis_result* get_child(size_t i) const;
	size_t get_size() const;

	/*******************************************************************/

	void build_request(size_t argc, const char* argv[], size_t lens[]);

	/*******************************************************************/

	void build(const char* cmd, const char* key,
		const std::map<string, string>& attrs);
	void build(const char* cmd, const char* key,
		const std::map<string, const char*>& attrs);

	void build(const char* cmd, const char* key,
		const std::map<int, string>& attrs);
	void build(const char* cmd, const char* key,
		const std::map<int, const char*>& attrs);

	void build(const char* cmd, const char* key,
		const std::vector<string>& names,
		const std::vector<string>& values);
	void build(const char* cmd, const char* key,
		const std::vector<const char*>& names,
		const std::vector<const char*>& values);

	void build(const char* cmd, const char* key,
		const std::vector<int>& names,
		const std::vector<string>& values);
	void build(const char* cmd, const char* key,
		const std::vector<int>& names,
		const std::vector<const char*>& values);

	void build(const char* cmd, const char* key,
		const char* names[], const char* values[], size_t argc);
	void build(const char* cmd, const char* key,
		const int names[], const char* values[], size_t argc);
	void build(const char* cmd, const char* key,
		const char* names[], const size_t names_len[],
		const char* values[], const size_t values_len[], size_t argc);

	/*******************************************************************/

	void build(const char* cmd, const char* key,
		const std::vector<string>& names);
	void build(const char* cmd, const char* key,
		const std::vector<const char*>& names);
	void build(const char* cmd, const char* key,
		const std::vector<int>& names);

	void build(const char* cmd, const char* key,
		const char* names[], size_t argc);
	void build(const char* cmd, const char* key,
		const int names[], size_t argc);
	void build(const char* cmd, const char* key,
		const char* names[], const size_t lens[], size_t argc);

	/*******************************************************************/

protected:
	// »ùÀàÐéº¯Êý
	virtual bool open();

private:
	bool slice_req_;
	unsigned long long used_;
	dbuf_pool* pool_;
	socket_stream conn_;
	char* addr_;
	int   conn_timeout_;
	int   rw_timeout_;
	bool  retry_;
	size_t  argv_size_;
	const char**  argv_;
	size_t* argv_lens_;
	size_t  argc_;
	string  request_;
	string  buf_;
	redis_request* req_;
	redis_result* result_;

	void argv_space(size_t n);

	redis_result* get_redis_objects(size_t nobjs);
	redis_result* get_redis_object();
	redis_result* get_redis_error();
	redis_result* get_redis_status();
	redis_result* get_redis_integer();
	redis_result* get_redis_string();
	redis_result* get_redis_array();

	void put_data(redis_result* rr, const char* data, size_t len);

	void build_request1(size_t argc, const char* argv[], size_t lens[]);
	void build_request2(size_t argc, const char* argv[], size_t lens[]);
	const redis_result* run1(size_t nchildren);
	const redis_result* run2(size_t nchildren);
};

} // end namespace acl
