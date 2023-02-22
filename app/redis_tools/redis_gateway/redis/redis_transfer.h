#pragma once

class redis_client;
class redis_object;

class redis_transfer {
public:
	redis_transfer(acl::dbuf_guard& dbuf, acl::socket_stream& conn,
		acl::redis_client_pipeline& pipeline);

	~redis_transfer(void);

	bool run(const std::vector<const redis_object*>& reqs);

private:
	acl::dbuf_guard&            dbuf_;
	acl::socket_stream&         conn_;
	acl::redis_client_pipeline& pipeline_;
	acl::string                 buff_;


	bool build_reply(const acl::redis_result& result, acl::string& buff);
	bool reply_add_object(const acl::redis_result& obj, acl::string& buff);
	bool reply_add_array(const acl::redis_result& obj, acl::string& buff);
	bool reply_add_string(const acl::redis_result& obj, acl::string& buff);
	bool reply_add_integer(const acl::redis_result& obj, acl::string& buff);
	bool reply_add_status(const acl::redis_result& obj, acl::string& buff);
	bool reply_add_error(const acl::redis_result& obj, acl::string& buff);

	bool redirect2me(void);
};
