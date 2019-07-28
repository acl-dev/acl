#include "lib_acl.h"
#include <string.h>
#include "lib_tpl.h"
#include "service_var.h"
#include "notify.h"

static int can_notify(ACL_CACHE *cache, const char *proc, const char *data)
{
	const char *ptr;

	if (cache == NULL)
		return (1);
	if (proc == NULL || *proc == 0) {
		acl_msg_warn("%s(%d): proc null", __FUNCTION__, __LINE__);
		return (0);
	}

	acl_cache_lock(cache);
	acl_cache_timeout(cache);
	ptr = acl_cache_find(cache, proc);
	if (ptr) {
		acl_cache_unlock(cache);
		acl_msg_warn("%s(%d): data(%s) has just been notified before!",
			__FUNCTION__, __LINE__, data);
		return (0);
	}

	acl_cache_enter(cache, proc, acl_mystrdup(proc));
	acl_cache_unlock(cache);

	return (1);
}

static int can_notify_sms(ACL_CACHE *cache, const char *proc, const char *data)
{
	time_t now;
	struct tm local_time;

	if (can_notify(cache, proc, data) == 0)
		return (0);

	(void) time(&now);
	(void) localtime_r(&now, &local_time);

	if (local_time.tm_wday >= var_cfg_work_week_min
		&& local_time.tm_wday <= var_cfg_work_week_max
		&& local_time.tm_hour >= var_cfg_work_hour_min
		&& local_time.tm_hour <= var_cfg_work_hour_max)
	{
		acl_msg_info("%s(%d): data(%s) no notify! tm_wday: %d, tm_hour: %d",
			__FUNCTION__, __LINE__, data,
			local_time.tm_wday, local_time.tm_hour);
		return (0);
	}

	return (1);
}

int notify(ACL_CACHE *smtp_notify_cache, ACL_CACHE *sms_notify_cache, const char *data)
{
	ACL_ARGV *args = acl_argv_split(data, "|");
	ACL_ITER iter;
	char *proc = NULL, *info = NULL;
	const char *ptr;
	ACL_ARGV *rcpts = NULL, *to_mails, *to_phones;
	int   pid = -1;

#undef	RETURN
#define	RETURN(x) do {  \
	if (proc)  \
		acl_myfree(proc);  \
	if (info)  \
		acl_myfree(info);  \
	if (rcpts)  \
		acl_argv_free(rcpts);  \
	acl_argv_free(args);  \
	return (x);  \
} while (0)

	if (args->argc < 3) {
		acl_msg_error("%s(%d): invalid data(%s)",
			__FUNCTION__, __LINE__, data);
		RETURN (-1);
	}

	acl_foreach(iter, args) {
		ptr = (const char*) iter.data;

		if (strncasecmp(ptr, "PROC=", 5) == 0) {
			ptr += 5;
			if (proc)
				acl_myfree(proc);
			proc = acl_mystrdup(ptr);
		} else if (strncasecmp(ptr, "PID=", 4) == 0) {
			ptr += 4;
			pid = atoi(ptr);
		} else if (strncasecmp(ptr, "RCPT=", 5) == 0) {
			ptr += 5;
			rcpts = acl_argv_split(ptr, ",;\t ");
		} else if (strncasecmp(ptr, "info=", 5) == 0) {
			if (info)
				acl_myfree(info);
			ptr += 5;
			info = acl_mystrdup(ptr);
		}
	}

	if (proc == NULL) {
		acl_msg_error("%s(%d): invalid data(%s), no PROC",
			__FUNCTION__, __LINE__, data);
		RETURN (-1);
	}

	to_mails = acl_argv_alloc(1);
	to_phones = acl_argv_alloc(1);
	acl_foreach(iter, rcpts) {
		char *to = (char*) iter.data;
		char *p = strchr(to, ':');
		if (p)
			*p++ = 0;
		to_mails->push_back(to_mails, to);

		/* 简单地判断是否是手机号 */
		if (p && strlen(p) == 11)
			to_phones->push_back(to_phones, p);
	}

	/* 邮件通知 */
	if (can_notify(smtp_notify_cache, proc, data))
		(void) smtp_notify(proc, to_mails, pid,
			info == NULL ? "program exception!" : info);
	else
		acl_msg_info("%s(%d): data(%s) not be send to smtp!",
			__FUNCTION__, __LINE__, data);

	/* 手机短信通知 */
	if (can_notify_sms(sms_notify_cache, proc, data))
		(void) sms_notify(proc, to_phones, pid,
			info == NULL ? "program exception!" : info);
	else
		acl_msg_info("%s(%d): data(%s) not be send to sms!",
			__FUNCTION__, __LINE__, data);

	acl_argv_free(to_phones);
	acl_argv_free(to_mails);

	RETURN (0);
}
