#pragma once

class http_client;

class http_stream : public acl::http_aclient
{
public:
	http_stream(acl::aio_handle& handle, http_client& client);
	~http_stream(void);

protected:
	// @override
	void destroy(void);

	// @override
	bool on_connect(void);

	// @override
	void on_disconnect(void);

	// @override
	void on_ns_failed(void);

	// @override
	void on_connect_timeout(void);

	// @override
	void on_connect_failed(void);

	// @override
	bool on_read_timeout(void);

protected:
	// @override
	bool on_http_res_hdr(const acl::http_header& header);

	// @override
	bool on_http_res_body(char* data, size_t dlen);

	// @override
	bool on_http_res_finish(bool success);

private:
	http_client& client_;
};
