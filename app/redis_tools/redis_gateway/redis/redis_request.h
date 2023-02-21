#pragma once

class redis_object;

class redis_request : public acl::dbuf_obj {
public:
	redis_request(acl::dbuf_guard* dbuf,
		acl::redis_client_pipeline* pipeline);
	~redis_request(void);

	void build_request(const redis_object& obj);

	size_t get_argc(void) const {
		return argc_;
	}

	const char** get_argv(void) const {
		return argv_;
	}

	size_t* get_lens(void) const {
		return lens_;
	}

	acl::redis_pipeline_message& get_message(void);

private:
	acl::dbuf_guard* dbuf_;
	acl::redis   cmd_;
	size_t       size_;
	size_t       argc_;
	const char** argv_;
	size_t      *lens_;

	void add_object(const redis_object& obj);
	void add_array(const redis_object& obj);
	void add_string(const redis_object& obj);
};
