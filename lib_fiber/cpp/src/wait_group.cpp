#include "stdafx.hpp"
#include "fiber/fiber_tbox.hpp"
#include "fiber/wait_group.hpp"

namespace acl {

wait_group::wait_group(void)
{
	count_ = 0;
	box_   = new acl::fiber_tbox<unsigned long>;
}

wait_group::~wait_group(void)
{
	delete box_;
}

void wait_group::add(size_t n)
{
	count_ += n;
}

void wait_group::done(void)
{
#ifdef	_DEBUG
	unsigned long* tid = new unsigned long;
	*tid = acl::thread::self();
	box_->push(tid);
#else
	box_->push(NULL);
#endif
}

size_t wait_group::wait(void)
{
	size_t i;
	for (i = 0; i < count_; i++) {
		bool found;
#ifdef	_DEBUG
		unsigned long* tid = box_->pop(-1, &found);
		assert(found);
		delete tid;
#else
		(void) box_->pop(-1, &found);
		assert(found);
#endif
	}
	return i;
}

} // namespace acl
