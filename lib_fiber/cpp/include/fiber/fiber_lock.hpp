#pragma once

struct ACL_FIBER_EVENT;
struct ACL_FIBER_RWLOCK;

namespace acl {

class thread_mutex;

class fiber_mutex
{
public:
	fiber_mutex(void);
	~fiber_mutex(void);

	bool lock(void);
	bool unlock(void);

private:
	unsigned long tid_;
	ACL_FIBER_EVENT* lock_;
};

class fiber_rwlock
{
public:
	fiber_rwlock(void);
	~fiber_rwlock(void);

	void rlock(void);
	bool tryrlock(void);
	void runlock(void);

	void wlock(void);
	bool trywlock(void);
	void wunlock(void);

private:
	ACL_FIBER_RWLOCK* rwlk_;
};

} // namespace acl
