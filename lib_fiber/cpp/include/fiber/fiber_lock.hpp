#pragma once

struct ACL_FIBER_MUTEX;
struct ACL_FIBER_RWLOCK;

namespace acl {

class thread_mutex;

class fiber_mutex
{
public:
	fiber_mutex(bool thread_safe = false);
	~fiber_mutex(void);

	bool lock(void);
	bool trylock(void);
	bool unlock(void);

private:
	thread_mutex* thread_lock_;
	ACL_FIBER_MUTEX* lock_;
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
