#include "stdafx.h"
#include "wait_group.h"

wait_group::wait_group(void) {
	box_ = new acl::fiber_tbox<unsigned long>;
}

wait_group::~wait_group(void) {
	delete box_;
}

void wait_group::add(size_t n) {
	count_ += n;
}

void wait_group::done(void) {
	unsigned long* tid = new unsigned long;
	*tid = acl::thread::self();
	box_->push(tid);
}

size_t wait_group::wait(void) {
	size_t i;
	for (i = 0; i < count_; i++) {
		bool found;
		unsigned long* tid = box_->pop(-1, &found);
		assert(found);
		printf("Got one, tid=%lu\r\n", *tid);
		delete tid;
	}
	return i;
}

