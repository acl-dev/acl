#pragma once

class tcp_transfer : public acl::fiber
{
public:
	tcp_transfer(ACL_FIBER* parent, acl::socket_stream& in,
		acl::socket_stream& out, bool running);
	~tcp_transfer(void);

	void set_peer(tcp_transfer* peer);
	void unset_peer(void);
	void close(void);
	void wait(void);

	void set_local(bool yes);

	ACL_FIBER* peer_fiber(void) const {
		return me_;
	}

public:
	// @override
	void run(void);

private:
	acl::fiber_tbox<int> box_;
	ACL_FIBER* parent_;
	ACL_FIBER* me_;
	acl::socket_stream& in_;
	acl::socket_stream& out_;
	tcp_transfer* peer_;
	bool is_local_;
};

