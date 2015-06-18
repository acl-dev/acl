#pragma once
#include "acl_cpp/acl_cpp_define.hpp"

struct ACL_AQUEUE;

namespace acl
{

class ACL_CPP_API thread_qitem
{
public:
	thread_qitem() {}
	virtual ~thread_qitem() {}
};

class ACL_CPP_API thread_queue
{
public:
	thread_queue();
	~thread_queue();

	bool push(thread_qitem* item);
	thread_qitem* pop(int wait_ms = -1);
	int qlen() const;

private:
	ACL_AQUEUE* queue_;
};

} // namespace acl
