#include "acl_stdafx.hpp"
#ifndef ACL_PREPARE_COMPILE
#include <assert.h>
#include "acl_cpp/stdlib/log.hpp"
#include "acl_cpp/stdlib/mbox.hpp"
#endif

namespace acl
{

void* mbox_create(void)
{
	return acl_mbox_create();
}

void mbox_free(void* mbox, void (*free_fn)(void*))
{
	acl_mbox_free((ACL_MBOX*) mbox, free_fn);
}

bool mbox_send(void* mbox, void* o)
{
	return acl_mbox_send((ACL_MBOX*) mbox, o) == 0;
}

void* mbox_read(void* mbox, int timeout, bool* success)
{
	int ok;
	void* o = (void*) acl_mbox_read((ACL_MBOX*) mbox, timeout, &ok);
	if (success) {
		*success = ok ? true : false;
	}
	return o;
}

size_t mbox_nsend(void* mbox)
{
	return acl_mbox_nsend((ACL_MBOX*) mbox);
}

size_t mbox_nread(void* mbox)
{
	return acl_mbox_nread((ACL_MBOX*) mbox);
}

} // namespace acl
