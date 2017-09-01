#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <assert.h>
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/mbox.hpp"
#endif

namespace acl
{

template<typename T>
mbox<T>::mbox(void)
{
	mbox_ = acl_mbox_create();
}

template<typename T>
mbox<T>::~mbox(void)
{
	acl_mbox_free(mbox_, NULL);
}

template<typename T>
bool mbox<T>::push_one(void* o)
{
	return acl_mbox_send(mbox_, o) == 0;
}

template<typename T>
void* mbox<T>::pop_one(int timeout /* = 0 */, bool* success /* = NULL */)
{
	int ok;
	void* o = (void*) acl_mbox_read(mbox_, timeout, &ok);
	if (success)
		*success = ok ? true : false;
	return o;
}

template<typename T>
size_t mbox<T>::push_count(void) const
{
	return acl_mbox_nsend(mbox_);
}

template<typename T>
size_t mbox<T>::pop_count(void) const
{
	return acl_mbox_nread(mbox_);
}

} // namespace acl
