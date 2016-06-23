#pragma once
#include "acl_cpp/lib_acl.hpp"

namespace acl {

class master_fiber : public acl::master_base
{
public:
	void run_daemon(int argc, char** argv);
	bool run_alone(const char* addrs, const char* path = NULL,
		unsigned int count = 0);

protected:
	master_fiber();
	virtual ~master_fiber();

	virtual void on_accept(socket_stream& stream) = 0;

private:
	static void service_main(ACL_VSTREAM*, void*);
	static int service_on_accept(ACL_VSTREAM*);
	static void service_pre_jail(void*);
	static void service_init(void*);
	static void service_exit(void*);
};

} // namespace acl
