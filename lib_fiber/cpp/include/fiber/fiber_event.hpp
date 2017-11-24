#pragma once

struct ACL_FIBER_EVENT;

namespace acl {

class thread_mutex;

class fiber_event
{
public:
	fiber_event(void);
	~fiber_event(void);

	bool wait(void);
	bool notify(void);

private:
	ACL_FIBER_EVENT* event_;
};

} // namespace acl
