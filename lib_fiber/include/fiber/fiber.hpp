#pragma once

struct FIBER;

namespace acl {

class fiber
{
public:
	fiber(void);
	virtual ~fiber(void);

	void start(size_t stack_size = 64000);

	int get_id(void) const;

	static int self(void);
	static void schedule();

protected:
	virtual void run(void) = 0;

private:
	FIBER *f_;

	static void fiber_callback(FIBER *f, void *ctx);
};

} // namespace acl
