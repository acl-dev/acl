#pragma once

class http_transfer : public acl::fiber
{
public:
	http_transfer(acl::http_method_t method, request_t& req,
		response_t& res, int port);
	~http_transfer(void);

	void wait(bool* keep_alive);

protected:
	// @override
	void run(void);

private:
	acl::fiber_tbox<bool>* box_;

	int port_;
	acl::http_method_t method_;
	request_t& req_;
	response_t& res_;
	acl::socket_stream conn_;
	acl::http_client* client_;

	acl::socket_stream req_in_;
	acl::socket_stream res_out_;
	acl::http_client* res_client_;

	bool open_peer(request_t& req, acl::socket_stream& conn);

	bool transfer_get(void);
	bool transfer_post(void);

	bool transfer_request_head(acl::socket_stream& conn);
	bool transfer_request_body(acl::socket_stream& conn);
	bool transfer_response(void);
};

