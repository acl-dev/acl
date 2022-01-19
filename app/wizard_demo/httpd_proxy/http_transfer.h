#pragma once

class http_transfer : public acl::fiber
{
public:
	http_transfer(request_t& req, response_t& res);
	~http_transfer(void);

	void set_peer(http_transfer& peer);
	void wait(void);

protected:
	// @override
	void run(void);

private:
	acl::fiber_tbox<int> box_;
	request_t& req_;
	response_t& res_;
	http_transfer* peer_;
};

