#pragma once

struct ACL_FIBER_SEM;

namespace acl {

class fiber_sem
{
public:
	fiber_sem(int max);
	~fiber_sem(void);

	int wait(void);
	int trywait(void);
	int post(void);

private:
	ACL_FIBER_SEM* sem_;
	fiber_sem(const fiber_sem&);
	const fiber_sem& operator=(const fiber_sem&);
};

class fiber_sem_guard
{
public:
	fiber_sem_guard(fiber_sem& sem) : sem_(sem)
	{
		(void) sem_.wait();
	}

	~fiber_sem_guard(void)
	{
		sem_.post();
	}

private:
	fiber_sem& sem_;
};

} // namespace acl
