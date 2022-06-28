#pragma once

class http_stream;

class http_client
{
public:
	http_client(acl::aio_handle& handle);

	bool start(void);

public:
	http_client& set_redirect_limit(int max);
	http_client& set_addr(const char* addr);
	http_client& set_timeout(int conn_timeout, int rw_timeout);
	http_client& set_debug(bool on);
	http_client& set_url(const char* url);
	http_client& set_host(const char* host);
	http_client& set_keep_alive(bool yes);

public:
	void on_destroy(http_stream* conn);
	void on_connect(http_stream& conn);
	void on_disconnect(http_stream& conn);
	void on_ns_failed(http_stream& conn);
	void on_connect_timeout(http_stream& conn);
	void on_connect_failed(http_stream& conn);
	bool on_read_timeout(http_stream& conn);

	bool on_http_res_hdr(http_stream& conn, const acl::http_header& header);
	bool on_http_res_body(http_stream& conn, char* data, size_t dlen);
	bool on_http_res_finish(http_stream& conn, bool success);

private:
	~http_client(void);

	bool redirect(const char* url);
	bool open(void);

private:
	acl::aio_handle& handle_;
	int refered_;

	acl::string addr_;
	int conn_timeout_;
	int rw_timeout_;
	bool debug_;

	int redirect_limit_;
	int redirect_count_;
	acl::string url_;
	acl::string host_;
	bool keep_alive_;
	bool compressed_;
};
