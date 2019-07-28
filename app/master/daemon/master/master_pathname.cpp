#include "stdafx.h"

#include "master_pathname.h"

/* acl_master_pathname - map service class and service name to pathname */

char *acl_master_pathname(const char *queue_path, const char *service_class,
	const char *service_name)
{
	return (acl_concatenate(queue_path, "/", service_class, "/",
			service_name, (char *) 0));
}
