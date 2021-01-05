#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/snprintf.hpp"
#include "acl_cpp/stdlib/dbuf_pool.hpp"
#include "acl_cpp/redis/redis_client.hpp"
#include "acl_cpp/redis/redis_client_pool.hpp"
#include "acl_cpp/redis/redis_client_cluster.hpp"
#include "acl_cpp/redis/redis_client_pipeline.hpp"
#include "acl_cpp/redis/redis_result.hpp"
#include "acl_cpp/redis/redis_command.hpp"
#endif
#include "redis_request.hpp"

#if !defined(ACL_CLIENT_ONLY) && !defined(ACL_REDIS_DISABLE)

namespace acl
{

#define INT_LEN		11
#define	LONG_LEN	21

void redis_command::init(void)
{
	check_addr_     = false;
	conn_           = NULL;
	cluster_        = NULL;
	pipeline_       = NULL;
	slot_           = -1;
	redirect_max_   = 15;
	redirect_sleep_ = 100;
	slice_req_      = false;
	request_buf_    = NULL;
	request_obj_    = NULL;
	argv_size_      = 0;
	argv_           = NULL;
	argv_lens_      = NULL;
	slice_res_      = false;
	result_         = NULL;
	pipe_msg_       = NULL;
	addr_[0]        = 0;
	dbuf_           = new dbuf_pool();
}

redis_command::redis_command(void)
{
	init();
}

redis_command::redis_command(redis_client* conn)
{
	init();
	conn_ = conn;

	if (conn != NULL) {
		set_client_addr(*conn);
	}
}

redis_command::redis_command(redis_client_cluster* cluster)
{
	init();
	cluster_   = cluster;

	if (cluster != NULL) {
		redirect_max_ = cluster->get_redirect_max();
		if (redirect_max_ <= 0)
			redirect_max_ = 15;
		redirect_sleep_ = cluster->get_redirect_sleep();
	} else {
		redirect_max_ = 15;
		redirect_sleep_ = 100;
	}
}

redis_command::redis_command(redis_client_cluster* cluster, size_t)
{
	init();
	cluster_   = cluster;

	if (cluster != NULL) {
		redirect_max_ = cluster->get_redirect_max();
		if (redirect_max_ <= 0) {
			redirect_max_ = 15;
		}
		redirect_sleep_ = cluster->get_redirect_sleep();
	} else {
		redirect_max_ = 15;
		redirect_sleep_ = 100;
	}
}

redis_command::redis_command(redis_client_pipeline* pipeline)
{
	init();
	pipeline_ = pipeline;
}

redis_command::~redis_command(void)
{
	if (argv_ != NULL) {
		acl_myfree(argv_);
	}
	if (argv_lens_ != NULL) {
		acl_myfree(argv_lens_);
	}
	delete request_buf_;
	delete request_obj_;
	delete pipe_msg_;
	dbuf_->destroy();
}

void redis_command::set_check_addr(bool on)
{
	check_addr_ = on;
}

void redis_command::reset(bool save_slot /* = false */)
{
	return clear(save_slot);
}

void redis_command::clear(bool save_slot /* = false */)
{
	dbuf_->dbuf_reset();
	result_ = NULL;

	if (!save_slot) {
		slot_ = -1;
	}
}

void redis_command::set_slice_request(bool on)
{
	slice_req_ = on;
}

void redis_command::set_slice_respond(bool on)
{
	slice_res_ = on;
}

void redis_command::set_client(redis_client* conn)
{
	if (conn != NULL) {
		conn_ = conn;
		cluster_ = NULL;
		set_client_addr(*conn);
	}
}

void redis_command::set_client_addr(redis_client& conn)
{
	socket_stream* stream = conn.get_stream();
	if (stream == NULL) {
		addr_[0] = 0;
	} else {
		ACL_SAFE_STRNCPY(addr_, stream->get_peer(true), sizeof(addr_));
	}
}

void redis_command::set_client_addr(const char* addr)
{
	ACL_SAFE_STRNCPY(addr_, addr, sizeof(addr_));
}

void redis_command::set_cluster(redis_client_cluster* cluster, size_t)
{
	set_cluster(cluster);
}

void redis_command::set_cluster(redis_client_cluster* cluster)
{
	cluster_ = cluster;
	if (cluster == NULL) {
		return;
	}

	conn_ = NULL;
	redirect_max_ = cluster->get_redirect_max();
	if (redirect_max_ <= 0) {
		redirect_max_ = 15;
	}
	redirect_sleep_ = cluster->get_redirect_sleep();
}

void redis_command::set_pipeline(redis_client_pipeline* pipeline)
{
	pipeline_ = pipeline;
}

// 分析重定向信息，获得重定向的服务器地址
const char* redis_command::get_addr(const char* info)
{
	char* cmd = dbuf_->dbuf_strdup(info);
	char* slot = strchr(cmd, ' ');
	if (slot == NULL) {
		return NULL;
	}
	*slot++ = 0;
	char* addr = strchr(slot, ' ');
	if (addr == NULL) {
		return NULL;
	}
	*addr++ = 0;
	if (*addr == 0) {
		return NULL;
	}

	return addr;
}

bool redis_command::eof(void) const
{
	return conn_ == NULL ? false : conn_->eof();
}

redis_pipeline_message& redis_command::get_pipeline_message(void)
{
	if (pipe_msg_ == NULL) {
		pipe_msg_ = NEW redis_pipeline_message(
			this, redis_pipeline_t_cmd);
	}
	return *pipe_msg_;
}

void redis_command::argv_space(size_t n)
{
	if (argv_size_ >= n) {
		return;
	}
	argv_size_ = n;
	if (argv_ == NULL) {
		argv_ = (const char**) acl_mymalloc(n * sizeof(char*));
		argv_lens_ = (size_t*) acl_mymalloc(n * sizeof(size_t));
	} else {
		argv_ = (const char**) acl_myrealloc(argv_, n * sizeof(char*));
		argv_lens_ = (size_t*) acl_myrealloc(argv_lens_,
			n * sizeof(size_t));
	}
}

void redis_command::hash_slot(const char* key)
{
	hash_slot(key, strlen(key));
}

void redis_command::hash_slot(const char* key, size_t len)
{
	int max_slot;

	// 只有集群模式才需要计算哈希槽值
	if (cluster_ != NULL) {
		max_slot = cluster_->get_max_slot();
	} else if (pipeline_ != NULL) {
		max_slot = pipeline_->get_max_slot();
	} else {
		return;
	}

	if (max_slot <= 0) {
		return;
	}

	// 如果缓存了哈希槽值，则不必重新计算
	if (slot_ >= 0 && slot_ < max_slot) {
		return;
	}

	unsigned short n = acl_hash_crc16(key, len);
	slot_ = (int) (n % max_slot);
}

const char* redis_command::get_client_addr(void) const
{
	return addr_;
}

/////////////////////////////////////////////////////////////////////////////

size_t redis_command::result_size(void) const
{
	return result_ ? result_->get_size() : 0;
}

redis_result_t redis_command::result_type(void) const
{
	return result_ ? result_->get_type() : REDIS_RESULT_UNKOWN;
}

int redis_command::result_number(bool* success /* = NULL */) const
{
	return result_ ? result_->get_integer(success) : 0;
}

long long int redis_command::result_number64(bool* success /* = NULL */) const
{
	return result_ ? result_->get_integer64(success) : 0;
}

const char* redis_command::get_result(size_t i, size_t* len /* = NULL */) const
{
	return result_ ? result_->get(i, len) : NULL;
}

const char* redis_command::result_status(void) const
{
	return result_ ? result_->get_status() : "";
}

const char* redis_command::result_error(void) const
{
	const char* ptr = result_ ? result_->get_error() : "";
	if (ptr && *ptr) {
		return ptr;
	}
	return last_serror();
}

const redis_result* redis_command::result_child(size_t i) const
{
	return result_ ? result_->get_child(i) : NULL;
}

const char* redis_command::result_value(size_t i, size_t* len /* = NULL */) const
{
	if (result_ == NULL || result_->get_type() != REDIS_RESULT_ARRAY) {
		return NULL;
	}
	const redis_result* child = result_->get_child(i);
	if (child == NULL) {
		return NULL;
	}
	size_t size = child->get_size();
	if (size == 0) {
		return NULL;
	}
	if (size == 1) {
		return child->get(0, len);
	}

	// 大内存有可能被切片成多个不连续的小内存
	size = child->get_length();
	size++;
	char* buf = (char*) dbuf_->dbuf_alloc(size);
	size = child->argv_to_string(buf, size);
	if (len) {
		*len = size;
	}
	return buf;
}

const redis_result* redis_command::get_result(void) const
{
	return result_;
}

const redis_result* redis_command::run(size_t nchild /* = 0 */,
	int* timeout /* = NULL */)
{
	if (pipeline_ != NULL) {
		redis_pipeline_message& msg = get_pipeline_message();
		msg.set_option(nchild, timeout);
		result_ = pipeline_->run(msg);

		return result_;
	} else if (cluster_ != NULL) {
		result_ = cluster_->run(*this, nchild, timeout);
		return result_;
	} else if (conn_ == NULL) {
		logger_error("ERROR: cluster_ and conn_ are all NULL");
		return NULL;
	}

	conn_->set_check_addr(check_addr_);

	if (slice_req_) {
		result_ = conn_->run(dbuf_, *request_obj_, nchild, timeout);
	} else {
		result_ = conn_->run(dbuf_, *request_buf_, nchild, timeout);
	}
	return result_;
}

/////////////////////////////////////////////////////////////////////////////

void redis_command::logger_result(const redis_result* result)
{
	if (result == NULL) {
		logger_error("result NULL");
		return;
	}

	string res;
	result->to_string(res);

	logger_error("result type: %d, error: %s, res: [%s], req:[%s]",
		result->get_type(), result_error(), res.c_str(),
		request_buf_ ? request_buf_->c_str() : "slice request");
}

int redis_command::get_number(bool* success /* = NULL */)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_INTEGER) {
		if (success) {
			*success = false;
		}
		logger_result(result);
		return -1;
	}
	if (success) {
		*success = true;
	}
	return result->get_integer();
}

long long int redis_command::get_number64(bool* success /* = NULL */)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_INTEGER) {
		if (success) {
			*success = false;
		}
		logger_result(result);
		return -1;
	}
	if (success) {
		*success = true;
	}
	return result->get_integer64();
}

int redis_command::get_number(std::vector<int>& out)
{
	out.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY) {
		logger_result(result);
		return -1;
	}

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0) {
		return 0;
	}
	out.reserve(size);

	const redis_result* rr;
	for (size_t i = 0; i < size; i++) {
		rr = children[i];
		out.push_back(rr->get_integer());
	}

	return (int) size;
}

int redis_command::get_number64(std::vector<long long int>& out)
{
	out.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY) {
		logger_result(result);
		return -1;
	}

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0) {
		return 0;
	}
	out.reserve(size);

	const redis_result* rr;
	for (size_t i = 0; i < size; i++) {
		rr = children[i];
		out.push_back(rr->get_integer64());
	}

	return (int) size;
}

bool redis_command::check_status(const char* success /* = "OK" */)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_STATUS) {
		logger_result(result);
		return false;
	}

	const char* status = result->get_status();
	if (status == NULL || *status == '\0') {
		return false;
	} else if (success == NULL || strcasecmp(status, success) == 0) {
		return true;
	} else {
		return false;
	}
}

int redis_command::get_status(std::vector<bool>& out)
{
	out.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY) {
		logger_result(result);
		return -1;
	}

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL || size == 0) {
		return 0;
	}

	out.reserve(size);

	const redis_result* rr;
	for (size_t i = 0; i < size; i++) {
		rr = children[i];
		out.push_back(rr->get_integer() > 0 ? true : false);
	}

	return (int) size;
}

const char* redis_command::get_status(void)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_STATUS) {
		logger_result(result);
		return "";
	}
	return result->get_status();
}

int redis_command::get_string(string& buf)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_STRING) {
		logger_result(result);
		return -1;
	}
	return result->argv_to_string(buf);
}

int redis_command::get_string(string* buf)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_STRING) {
		logger_result(result);
		return -1;
	}
	if (buf == NULL) {
		return (int) result->get_length();
	}
	return result->argv_to_string(*buf);
}

int redis_command::get_string(char* buf, size_t size)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_STRING) {
		logger_result(result);
		return -1;
	}
	return result->argv_to_string(buf, size);
}

int redis_command::get_strings(std::vector<string>& out)
{
	return get_strings(&out);
}

int redis_command::get_strings(std::vector<string>* out)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY) {
		logger_result(result);
		return -1;
	}

	if (out == NULL) {
		return (int) result->get_size();
	}

	out->clear();

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL) {
		return 0;
	}

	if (size > 0) {
		out->reserve(size);
	}

	const redis_result* rr;
	string buf(4096);

	for (size_t i = 0; i < size; i++) {
		rr = children[i];
		if (rr == NULL || rr->get_type() != REDIS_RESULT_STRING) {
			out->push_back("");
		} else if (rr->get_size() == 0) {
			out->push_back("");
		} else {
			rr->argv_to_string(buf);
			out->push_back(buf);
			buf.clear();
		}
	}

	return (int) size;
}


int redis_command::get_strings(std::list<string>& out)
{
	return get_strings(&out);
}

int redis_command::get_strings(std::list<string>* out)
{
	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY) {
		logger_result(result);
		return -1;
	}
	if (out == NULL) {
		return (int) result->get_size();
	}

	out->clear();

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL) {
		return 0;
	}

	const redis_result* rr;
	string buf(4096);

	for (size_t i = 0; i < size; i++) {
		rr = children[i];
		if (rr == NULL || rr->get_type() != REDIS_RESULT_STRING) {
			out->push_back("");
		} else if (rr->get_size() == 0) {
			out->push_back("");
		} else {
			rr->argv_to_string(buf);
			out->push_back(buf);
			buf.clear();
		}
	}

	return (int) size;
}

int redis_command::get_strings(std::map<string, string>& out)
{
	out.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY) {
		logger_result(result);
		return -1;
	}
	if (result->get_size() == 0) {
		return 0;
	}

	size_t size;
	const redis_result** children = result->get_children(&size);
	if (children == NULL) {
		return -1;
	}
	if (size % 2 != 0) {
		return -1;
	}

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
		if (rr->get_type() != REDIS_RESULT_STRING) {
			i++;
			continue;
		}
		value.clear();
		rr->argv_to_string(value);
		i++;

		out[name] = value;
	}

	return (int) out.size();
}

int redis_command::get_strings(std::vector<string>& names,
	std::vector<string>& values)
{
	names.clear();
	values.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY) {
		logger_result(result);
		return -1;
	}
	if (result->get_size() == 0) {
		return 0;
	}

	size_t size;
	const redis_result** children = result->get_children(&size);

	if (children == NULL) {
		return -1;
	}
	if (size % 2 != 0) {
		return -1;
	}

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
		if (rr->get_type() != REDIS_RESULT_STRING) {
			i++;
			continue;
		}
		value.clear();
		rr->argv_to_string(value);
		i++;

		names.push_back(name);
		values.push_back(value);
	}

	return (int) names.size();
}

int redis_command::get_strings(std::vector<const char*>& names,
	std::vector<const char*>& values)
{
	names.clear();
	values.clear();

	const redis_result* result = run();
	if (result == NULL || result->get_type() != REDIS_RESULT_ARRAY) {
		logger_result(result);
		return -1;
	}
	if (result->get_size() == 0) {
		return 0;
	}

	size_t size;
	const redis_result** children = result->get_children(&size);

	if (children == NULL) {
		return -1;
	}
	if (size % 2 != 0) {
		return -1;
	}

	char* nbuf, *vbuf;
	size_t len;
	const redis_result* rr;

	for (size_t i = 0; i < size;) {
		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_STRING) {
			i += 2;
			continue;
		}
		len = rr->get_length() + 1;
		nbuf = (char*) dbuf_->dbuf_alloc(len);
		rr->argv_to_string(nbuf, len);
		i++;

		rr = children[i];
		if (rr->get_type() != REDIS_RESULT_STRING) {
			i++;
			continue;
		}
		len = rr->get_length() + 1;
		vbuf = (char*) dbuf_->dbuf_alloc(len);
		rr->argv_to_string(vbuf, len);
		i++;

		names.push_back(nbuf);
		values.push_back(vbuf);
	}

	return (int) names.size();
}

/////////////////////////////////////////////////////////////////////////////

const redis_result** redis_command::scan_keys(const char* cmd, const char* key,
	int& cursor, size_t& size, const char* pattern, const size_t* count)
{
	size_t klen = key ? strlen(key) : 0;
	return scan_keys(cmd, key, klen, cursor, size, pattern, count);
}

const redis_result** redis_command::scan_keys(const char* cmd, const char* key,
	size_t klen, int& cursor, size_t& size, const char* pattern,
	const size_t* count)
{
	size = 0;
	if (cursor < 0) {
		return NULL;
	}

	const char* argv[7];
	size_t lens[7];
	size_t argc = 0;

	argv[argc] = cmd;
	lens[argc] = strlen(cmd);
	argc++;

	if (key && *key && klen > 0) {
		argv[argc] = key;
		lens[argc] = klen;
		argc++;
	}

	char cursor_s[INT_LEN];
	safe_snprintf(cursor_s, sizeof(cursor_s), "%d", cursor);
	argv[argc] = cursor_s;
	lens[argc] = strlen(cursor_s);
	argc++;

	if (pattern && *pattern) {
		argv[argc] = "MATCH";
		lens[argc] = sizeof("MATCH") - 1;
		argc++;

		argv[argc] = pattern;
		lens[argc] = strlen(pattern);
		argc++;
	}

	if (count && *count > 0) {
		argv[argc] = "COUNT";
		lens[argc] = sizeof("COUNT") - 1;
		argc++;

		char count_s[LONG_LEN];
		safe_snprintf(count_s, sizeof(count_s), "%lu",
			(unsigned long) (*count));
		argv[argc] = count_s;
		lens[argc] = strlen(count_s);
		argc++;
	}

	if (key && *key) {
		hash_slot(key);
	}
	build_request(argc, argv, lens);
	const redis_result* result = run();
	if (result == NULL) {
		cursor = -1;
		return NULL;
	}

	if (result->get_size() != 2) {
		cursor = -1;
		return NULL;
	}

	const redis_result* rr = result->get_child(0);
	if (rr == NULL) {
		cursor = -1;
		return NULL;
	}
	string tmp(128);
	if (rr->argv_to_string(tmp) <= 0) {
		cursor = -1;
		return NULL;
	}
	cursor = atoi(tmp.c_str());
	if (cursor < 0) {
		cursor = -1;
		return NULL;
	}

	rr = result->get_child(1);
	if (rr == NULL) {
		cursor = -1;
		return NULL;
	}

	const redis_result** children = rr->get_children(&size);
	if (children == NULL) {
		//cursor = 0;
		size = 0;
	}

	return children;
}

void redis_command::clear_request(void)
{
	if (request_buf_) {
		request_buf_->clear();
	}
	if (request_obj_) {
		request_obj_->clear();
	}
}

void redis_command::build_request(size_t argc, const char* argv[], size_t lens[])
{
	if (pipeline_) {
		redis_pipeline_message& msg = get_pipeline_message();
		//build_request1(argc, argv, lens);
		msg.set_request(argc, argv, lens);
	} else if (slice_req_) {
		build_request2(argc, argv, lens);
	} else {
		build_request1(argc, argv, lens);
	}
}

void redis_command::build_request1(size_t argc, const char* argv[], size_t lens[])
{
	if (request_buf_ == NULL) {
		request_buf_ = NEW string(256);
	} else {
		request_buf_->clear();
	}
	build_request(argc, argv, lens, *request_buf_);
}

void redis_command::build_request(size_t argc, const char* argv[],
	size_t lens[], string& out)
{
//#define	USE_FORMAT
//#define	USE_SNPRINTF

#if	defined(USE_FORMAT)
	out.format("*%lu\r\n", (unsigned long) argc);
#elif	defined(USE_SNPRINTF)
	char  buf[64];
	snprintf(buf, sizeof(buf), "*%lu\r\n", (unsigned long) argc);
	out.append(buf);
#else
	char  buf[64];
	acl_ui64toa_radix((acl_uint64) argc, buf, sizeof(buf), 10);
	out.append("*");
	out.append(buf);
	out.append("\r\n");
#endif

	for (size_t i = 0; i < argc; i++) {
#if	defined(USE_FORMAT)
		out.format_append("$%lu\r\n", (unsigned long) lens[i]);
#elif	defined(USE_SNPRINTF)
		snprintf(buf, sizeof(buf), "$%lu\r\n", (unsigned long) lens[i]);
		out.append(buf);
#else
		acl_ui64toa_radix((acl_uint64) lens[i], buf, sizeof(buf), 10);
		out.append("$");
		out.append(buf);
		out.append("\r\n");
#endif
		out.append(argv[i], lens[i]);
		out.append("\r\n");
	}

	//printf("%s:\r\n%s\r\n", __FUNCTION__, out.c_str());
}

void redis_command::build_request2(size_t argc, const char* argv[], size_t lens[])
{
	size_t size = 1 + argc * 3;
	if (request_obj_ == NULL) {
		request_obj_ = NEW redis_request();
	} else {
		request_obj_->clear();
	}
	request_obj_->reserve(size);

#define BLEN	32

	char* buf = (char*) dbuf_->dbuf_alloc(BLEN);
	int  len = safe_snprintf(buf, BLEN, "*%lu\r\n", (unsigned long) argc);
	request_obj_->put(buf, len);

	for (size_t i = 0; i < argc; i++) {
		buf = (char*) dbuf_->dbuf_alloc(BLEN);
		len = safe_snprintf(buf, BLEN, "$%lu\r\n",
			(unsigned long) lens[i]);
		request_obj_->put(buf, len);

		request_obj_->put(argv[i], lens[i]);

		buf = (char*) dbuf_->dbuf_strdup("\r\n");
		request_obj_->put(buf, 2);
	}
}

//////////////////////////////////////////////////////////////////////////

void redis_command::build(const char* cmd, const char* key,
	const std::map<string, string>& attrs)
{
	build(cmd, key, key ? strlen(key) : 0, attrs);
}

void redis_command::build(const char* cmd, const char* key, size_t klen,
	const std::map<string, string>& attrs)
{
	argc_ = 1 + attrs.size() * 2;
	if (key != NULL) {
		argc_++;
	}
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key && klen > 0) {
		argv_[i] = key;
		argv_lens_[i] = klen;
		i++;
	}

	std::map<string, string>::const_iterator cit = attrs.begin();
	for (; cit != attrs.end(); ++cit) {
		argv_[i] = cit->first.c_str();
		argv_lens_[i] = cit->first.size();
		i++;

		argv_[i] = cit->second.c_str();
		argv_lens_[i] = cit->second.size();
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const std::map<string, const char*>& attrs)
{
	argc_ = 1 + attrs.size() * 2;
	if (key != NULL) {
		argc_++;
	}
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL) {
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	std::map<string, const char*>::const_iterator cit = attrs.begin();
	for (; cit != attrs.end(); ++cit) {
		argv_[i] = cit->first.c_str();
		argv_lens_[i] = cit->first.size();
		i++;

		argv_[i] = cit->second;
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

/////////////////////////////////////////////////////////////////////////////

void redis_command::build(const char* cmd, const char* key,
	const std::vector<string>& names, const std::vector<string>& values)
{
	build(cmd, key, key ? strlen(key) : 0, names, values);
}

void redis_command::build(const char* cmd, const char* key, size_t klen,
	const std::vector<string>& names, const std::vector<string>& values)
{
	if (names.size() != values.size()) {
		logger_fatal("names's size: %lu, values's size: %lu",
			(unsigned long) names.size(),
			(unsigned long) values.size());
	}

	argc_ = 1 + names.size() * 2;
	if (key != NULL) {
		argc_++;
	}
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL && klen > 0) {
		argv_[i] = key;
		argv_lens_[i] = klen;
		i++;
	}

	size_t size = names.size();
	for (size_t j = 0; j < size; j++) {
		argv_[i] = names[j].c_str();
		argv_lens_[i] = names[j].size();
		i++;

		argv_[i] = values[j].c_str();
		argv_lens_[i] = values[j].size();
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const std::vector<const char*>& names,
	const std::vector<const char*>& values)
{
	if (names.size() != values.size()) {
		logger_fatal("names's size: %lu, values's size: %lu",
			(unsigned long) names.size(),
			(unsigned long) values.size());
	}

	argc_ = 1 + names.size() * 2;
	if (key != NULL) {
		argc_++;
	}
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL) {
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	size_t size = names.size();
	for (size_t j = 0; j < size; j++) {
		argv_[i] = names[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

/////////////////////////////////////////////////////////////////////////////

void redis_command::build(const char* cmd, const char* key,
	const char* names[], const char* values[], size_t argc)
{
	argc_ = 1 + argc * 2;
	if (key != NULL) {
		argc_++;
	}
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL) {
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++) {
		argv_[i] = names[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const int names[], const char* values[], size_t argc)
{
	argc_ = 1 + argc * 2;
	if (key != NULL) {
		argc_++;
	}
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL) {
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	char* buf4int;
	for (size_t j = 0; j < argc; j++) {
		buf4int = (char*) dbuf_->dbuf_alloc(INT_LEN);
		(void) safe_snprintf(buf4int, INT_LEN, "%d", names[j]);
		argv_[i] = buf4int;
		argv_lens_[i] = strlen(argv_[i]);
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const char* names[], const size_t names_len[],
	const char* values[], const size_t values_len[], size_t argc)
{
	build(cmd, key, key ? strlen(key) : 0, names, names_len,
		values, values_len, argc);
}

void redis_command::build(const char* cmd, const char* key, size_t klen,
	const char* names[], const size_t names_len[],
	const char* values[], const size_t values_len[], size_t argc)
{
	argc_ = 1 + argc * 2;
	if (key != NULL) {
		argc_++;
	}
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL && klen > 0) {
		argv_[i] = key;
		argv_lens_[i] = klen;
		i++;
	}

	for (size_t j = 0; j < argc; j++) {
		argv_[i] = names[j];
		argv_lens_[i] = names_len[j];
		i++;

		argv_[i] = values[j];
		argv_lens_[i] = values_len[j];
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

/////////////////////////////////////////////////////////////////////////////

void redis_command::build(const char* cmd, const char* key,
	const std::vector<string>& names)
{
	build(cmd, key, key ? strlen(key) : 0, names);
}

void redis_command::build(const char* cmd, const char* key, size_t klen,
	const std::vector<string>& names)
{
	size_t argc = names.size();
	argc_ = 1 + argc;
	if (key != NULL) {
		argc_++;
	}
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL && klen > 0) {
		argv_[i] = key;
		argv_lens_[i] = klen;
		i++;
	}

	for (size_t j = 0; j < argc; j++) {
		argv_[i] = names[j].c_str();
		argv_lens_[i] = names[j].size();
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const std::vector<const char*>& names)
{
	size_t argc = names.size();
	argc_ = 1 + argc;
	if (key != NULL) {
		argc_++;
	}
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL) {
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++) {
		argv_[i] = names[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const std::vector<int>& names)
{
	size_t argc = names.size();
	argc_ = 1 + argc;
	if (key != NULL) {
		argc_++;
	}
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL) {
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	char* buf4int;
	for (size_t j = 0; j < argc; j++) {
		buf4int = (char*) dbuf_->dbuf_alloc(INT_LEN);
		safe_snprintf(buf4int, INT_LEN, "%d", names[j]);
		argv_[i] = buf4int;
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const char* names[], size_t argc)
{
	argc_ = 1 + argc;
	if (key != NULL) {
		argc_++;
	}
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL) {
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	for (size_t j = 0; j < argc; j++) {
		argv_[i] = names[j];
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}


void redis_command::build(const char* cmd, const char* key,
	const char* names[], const size_t lens[], size_t argc)
{
	build(cmd, key, key ? strlen(key) : 0, names, lens, argc);
}

void redis_command::build(const char* cmd, const char* key, size_t klen,
	const char* names[], const size_t lens[], size_t argc)
{
	argc_ = 1 + argc;
	if (key != NULL) {
		argc_++;
	}
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL && klen > 0) {
		argv_[i] = key;
		argv_lens_[i] = klen;
		i++;
	}

	for (size_t j = 0; j < argc; j++) {
		argv_[i] = names[j];
		argv_lens_[i] = lens[j];
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

void redis_command::build(const char* cmd, const char* key,
	const int names[], size_t argc)
{
	argc_ = 1 + argc;
	if (key != NULL) {
		argc_++;
	}
	argv_space(argc_);

	size_t i = 0;
	argv_[i] = cmd;
	argv_lens_[i] = strlen(cmd);
	i++;

	if (key != NULL) {
		argv_[i] = key;
		argv_lens_[i] = strlen(key);
		i++;
	}

	char* buf4int;
	for (size_t j = 0; j < argc; j++) {
		buf4int = (char*) dbuf_->dbuf_alloc(INT_LEN);
		safe_snprintf(buf4int, INT_LEN, "%d", names[j]);
		argv_[i] = buf4int;
		argv_lens_[i] = strlen(argv_[i]);
		i++;
	}

	build_request(argc_, argv_, argv_lens_);
}

/////////////////////////////////////////////////////////////////////////////

const redis_result* redis_command::request(size_t argc, const char* argv[],
	size_t lens[], size_t nchild /* = 0 */)
{
	build_request(argc, argv, lens);
	const redis_result* result = run(nchild);
	return result;
}

const redis_result* redis_command::request(const std::vector<string>& args,
	size_t nchild /* = 0 */)
{
	argc_ = args.size();
	if (argc_ == 0) {
		logger_error("args empty!");
		return NULL;
	}

	argv_space(argc_);

	for (size_t i = 0; i < argc_; i++) {
		argv_[i] = args[i].c_str();
		argv_lens_[i] = args[i].size();
	}

	return request(argc_, argv_, argv_lens_, nchild);
}

/////////////////////////////////////////////////////////////////////////////

} // namespace acl

#endif // ACL_CLIENT_ONLY
