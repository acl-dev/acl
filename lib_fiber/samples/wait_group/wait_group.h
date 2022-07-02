#pragma once

class wait_group {
public:
	wait_group(void);
	~wait_group(void);

	void add(size_t n);
	void done(void);
	size_t wait(void);

private:
	size_t count_;
	acl::fiber_tbox<unsigned long>* box_;
};
