#pragma once

class master_service : public acl::master_fiber
{
public:
	master_service(void);
	~master_service(void);

protected:
	// @override
	void on_accept(acl::socket_stream& conn);

	// @override
	void proc_pre_jail(void);

	// @override
	void proc_on_init(void);

	// @override
	void proc_on_exit(void);

private:
};
