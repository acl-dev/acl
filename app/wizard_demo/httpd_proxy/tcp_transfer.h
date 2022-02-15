#pragma once

class tcp_transfer : public acl::fiber
{
public:
	tcp_transfer(acl::socket_stream& in, acl::socket_stream& out, bool running);
	~tcp_transfer(void);

	void set_peer(tcp_transfer& peer);
	void wait(void);

	acl::socket_stream& get_input(void) const
	{
		return in_;
	}

	acl::socket_stream& get_output(void) const
	{
		return out_;
	}

public:
	// @override
	void run(void);

private:
	acl::fiber_tbox<int> box_;
	acl::socket_stream& in_;
	acl::socket_stream& out_;
	tcp_transfer* peer_;
};

