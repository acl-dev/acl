#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE
#include "stdlib/acl_mymalloc.h"
#endif
#include "fiber/filber.h"

ACL_FIBER *acl_fiber_create(void)
{
	ACL_FIBER *fiber = (ACL_FIBER *) acl_mycalloc(1, sizeof(ACL_FIBER));
	return fiber;
}
