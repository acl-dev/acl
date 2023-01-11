#pragma once

struct ACL_FIBER_MUTEX;

namespace acl {

class fiber;

struct FIBER_CPP_API fiber_mutex_stat
{
	fiber_mutex_stat(void) : fb(NULL), waiting(NULL) {}
	~fiber_mutex_stat(void) {}

	fiber *fb;
	ACL_FIBER_MUTEX *waiting;
	std::vector<ACL_FIBER_MUTEX*> holding;
};

struct FIBER_CPP_API fiber_mutex_stats
{
	fiber_mutex_stats(void) {}
	~fiber_mutex_stats(void);

	std::vector<fiber_mutex_stat> stats;
};

} // namespace acl
