#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include "acl_cpp/stdlib/thread_queue.hpp"
#endif

namespace acl
{

thread_queue::thread_queue(void)
{
	queue_ = (ACL_AQUEUE*) acl_aqueue_new();
}

static void free_qitem(void* item)
{
	thread_qitem* qitem = (thread_qitem *) item;
	delete qitem;
}

thread_queue::~thread_queue(void)
{
	acl_aqueue_free(queue_, free_qitem);
}

bool thread_queue::push(thread_qitem* item)
{
	return acl_aqueue_push(queue_, item) == -1 ? false : true;
}

thread_qitem* thread_queue::pop(int wait_ms /* = -1 */)
{
	int wait_sec = wait_ms / 1000;
	int wait_usec = (wait_ms % 1000) * 1000;
	return (thread_qitem*) acl_aqueue_pop_timedwait(
				queue_, wait_sec, wait_usec);
}

int thread_queue::qlen(void) const
{
	return acl_aqueue_qlen(queue_);
}

//////////////////////////////////////////////////////////////////////////////

#if 0

void* tbox_create(void)
{
	return acl_aqueue_new();
}

void tbox_free(void* tbox, void (*free_fn)(void*))
{
	acl_aqueue_free((ACL_AQUEUE*) tbox, free_fn);
}

bool tbox_push(void* tbox, void* o)
{
	return acl_aqueue_push((ACL_AQUEUE*) tbox, o) == -1 ? false : true;
}

void* tbox_pop(void* tbox, int wait_ms /* = -1 */)
{
	int wait_sec = wait_ms / 1000;
	int wait_usec = (wait_ms % 1000) * 1000;
	return acl_aqueue_pop_timedwait((ACL_AQUEUE*) tbox,
			wait_sec, wait_usec);
}

size_t tbox_size(void* tbox)
{
	return (size_t) acl_aqueue_qlen((ACL_AQUEUE*) tbox);
}

#endif

} // namespace acl
