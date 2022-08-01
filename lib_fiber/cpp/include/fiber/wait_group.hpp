#pragma once

namespace acl {

template<typename T> class fiber_tbox;

class wait_group {
public:
	wait_group(void);
	~wait_group(void);

	void add(size_t n);
	void done(void);
	size_t wait(void);

private:
	size_t count_;
	fiber_tbox<unsigned long>* box_;
};

} // namespace acl
