#ifndef __NOTIFY_INCLUDE_H__
#define __NOTIFY_INCLUDE_H__

#include "lib_acl.h"

int notify(ACL_CACHE *smtp_cache, ACL_CACHE *sms_cache, const char *data);
int smtp_notify(const char *proc, ACL_ARGV *rcpts,
	int pid, const char *info);
int sms_notify(const char *proc, ACL_ARGV *rcpts,
	int pid, const char *info);

#endif
