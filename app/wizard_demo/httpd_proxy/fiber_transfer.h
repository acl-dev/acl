#pragma once

class fiber_transfer : public acl::fiber
{
public:
	fiber_transfer(acl::socket_stream& in, acl::socket_stream& out);
	~fiber_transfer(void);

	void set_peer(fiber_transfer& peer);
	void wait(void);

	acl::socket_stream& get_input(void) const
	{
		return in_;
	}

	acl::socket_stream& get_output(void) const
	{
		return out_;
	}

protected:
	// @override
	void run(void);

private:
	acl::fiber_tbox<int> box_;
	acl::socket_stream& in_;
	acl::socket_stream& out_;
	fiber_transfer* peer_;
};

