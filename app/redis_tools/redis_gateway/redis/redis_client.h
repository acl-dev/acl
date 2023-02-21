#pragma once

class redis_object;

class redis_client {
public:
	redis_client(acl::socket_stream& conn);
	~redis_client(void);

	void set_ssl_conf(acl::sslbase_conf* ssl_conf);
	bool eof(void) const;

	const redis_object* read_reply(acl::dbuf_pool& dbuf,
		size_t nchildren = 0, int* rw_timeout = NULL);
	bool read_request(acl::dbuf_pool& dbuf,
		std::vector<const redis_object*>& out);

private:
	acl::socket_stream& conn_;
	acl::sslbase_conf* ssl_conf_;
	acl::string* buff_;
	const std::vector<acl::string>* tokens_;

	void put_data(acl::dbuf_pool* dbuf, redis_object* obj,
		const char* data, size_t len);
	redis_object* get_error(acl::socket_stream& conn, acl::dbuf_pool* dbuf);
	redis_object* get_status(acl::socket_stream& conn, acl::dbuf_pool* dbuf);
	redis_object* get_integer(acl::socket_stream& conn, acl::dbuf_pool* dbuf);
	redis_object* get_string(acl::socket_stream& conn, acl::dbuf_pool* dbuf);
	redis_object* get_array(acl::socket_stream& conn, acl::dbuf_pool* dbuf);
	redis_object* get_object(acl::socket_stream& conn, acl::dbuf_pool* dbuf);
	redis_object* get_objects(acl::socket_stream& conn,
		acl::dbuf_pool* dbuf, size_t nobjs);

	redis_object* get_line(acl::socket_stream& conn, acl::dbuf_pool* dbuf);
};
