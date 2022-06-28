
#include "lib_acl.h"
#include <assert.h>
#include "app_log.h"

#ifdef HAS_LIB_CORE
#include "e_config.h"
#include "elib.h"

typedef struct LOG_WRAP {
	E_LOG_T *h_log;
	char  filename[1024];
} LOG_WRAP;

static LOG_WRAP *__log_wrap = NULL;

static int __log_open(const char *filename, void *ctx)
{
	LOG_WRAP *h_log = (LOG_WRAP *) ctx;
	int   logme = 0;
	char *facility_name = NULL;
	E_LOG_PRIORITY_T priority = E_LOG_INFO;
	E_LOG_ACTION_T action = E_LOG_PER_DAY;
	int   flush = 1;
	size_t limit_size = 0;
	E_LOG_SYNC_ACTION_T sync_action = E_LOG_SEM_WITH_MT;
	char *sem_name = NULL; 
	char *ptr, *pname;
	ACL_ARGV *env_argv;
	ACL_VSTRING *log_buf;
	int   i;

	if (filename == NULL || *filename == 0)
		return (-1);

	acl_snprintf(h_log->filename, sizeof(h_log->filename), "%s", filename);

	/* env: facility:x, priority:x, action:x, flush:x, limit_size:x, sync_action:x, sem_name:x */

	ptr = getenv("SERVICE_ENV");
	if (ptr == NULL)
		return (-1);

	env_argv = acl_argv_split(ptr, ",\t ");
	if (env_argv == NULL)
		return (-1);
	if (env_argv->argc == 0) {
		acl_argv_free(env_argv);
		return (-1);
	}

	log_buf = acl_vstring_alloc(256);

	for (i = 0; i < env_argv->argc; i++) {
		pname = acl_argv_index(env_argv, i);
		ptr = strchr(pname, ':');
		if (ptr == NULL)
			continue;
		*ptr++ = 0;
		if (*ptr == 0)
			continue;

		if (i == 0)
			acl_vstring_sprintf(log_buf, "%s:%s", pname, ptr);
		else
			acl_vstring_sprintf_append(log_buf, ", %s:%s", pname, ptr);

		if (strcasecmp(pname, "logme") == 0) {
			if (strcasecmp(ptr, "TRUE") == 0)
				logme = 1;
		} else if (strcasecmp(pname, "facility") == 0) {
			facility_name = ptr;
		} else if (strcasecmp(pname, "priority") == 0) {
			if (strcasecmp(ptr, "E_LOG_NOLOG") == 0)
				priority = E_LOG_NOLOG;
			else if (strcasecmp(ptr, "E_LOG_EMERG") == 0)
				priority = E_LOG_EMERG;
			else if (strcasecmp(ptr, "E_LOG_ALERT") == 0)
				priority = E_LOG_ALERT;
			else if (strcasecmp(ptr, "E_LOG_CRIT") == 0)
				priority = E_LOG_CRIT;
			else if (strcasecmp(ptr, "E_LOG_ERR") == 0)
				priority = E_LOG_ERR;
			else if (strcasecmp(ptr, "E_LOG_WARNING") == 0)
				priority = E_LOG_WARNING;
			else if (strcasecmp(ptr, "E_LOG_NOTICE") == 0)
				priority = E_LOG_NOTICE;
			else if (strcasecmp(ptr, "E_LOG_INFO") == 0)
				priority = E_LOG_INFO;
			else if (strcasecmp(ptr, "E_LOG_DEBUG") == 0)
				priority = E_LOG_DEBUG;
		} else if (strcasecmp(pname, "action") == 0) {
			if (strcasecmp(ptr, "E_LOG_PER_HOUR") == 0)
				action = E_LOG_PER_HOUR;
			else if (strcasecmp(ptr, "E_LOG_PER_DAY") == 0)
				action = E_LOG_PER_DAY;
			else if (strcasecmp(ptr, "E_LOG_PER_WEEK") == 0)
				action = E_LOG_PER_WEEK;
			else if (strcasecmp(ptr, "E_LOG_PER_MONTH") == 0)
				action = E_LOG_PER_MONTH;
			else if (strcasecmp(ptr, "E_LOG_PER_YEAR") == 0)
				action = E_LOG_PER_YEAR;
			else if (strcasecmp(ptr, "E_LOG_LIMIT_SIZE") == 0)
				action = E_LOG_LIMIT_SIZE;
			else if (strcasecmp(ptr, "E_LOG_SYSLOG") == 0)
				action = E_LOG_SYSLOG;
		} else if (strcasecmp(pname, "flush") == 0) {
			if (strcasecmp(ptr, "sync_flush") == 0)
				flush = 1;
			else if (strcasecmp(ptr, "async_flush") == 0)
				flush = 0;
		} else if (strcasecmp(pname, "limit_size") == 0) {
			limit_size = atoi(ptr);
		} else if (strcasecmp(pname, "sync_action") == 0) {
			if (strcasecmp(ptr, "E_LOG_NO_SYNC") == 0)
				sync_action = E_LOG_NO_SYNC;
			else if (strcasecmp(ptr, "E_LOG_THREAD_MUTEX") == 0)
				sync_action = E_LOG_THREAD_MUTEX;
			else if (strcasecmp(ptr, "E_LOG_FILE_LOCK") == 0)
				sync_action = E_LOG_FILE_LOCK;
			else if (strcasecmp(ptr, "E_LOG_SEM_WITH_MT") == 0)
				sync_action = E_LOG_SEM_WITH_MT;
			else if (strcasecmp(ptr, "E_LOG_FILE_APPEND_WITH_MT") == 0)
				sync_action = E_LOG_FILE_APPEND_WITH_MT;
		} else if (strcasecmp(pname, "sem_name") == 0) {
			sem_name = ptr;
		}
	}


#if 0
	LC_SysLogCreate(&h_log->h_log, h_log->filename);
#endif
	if (action == E_LOG_LIMIT_SIZE) {
		if (limit_size == 0)
			limit_size = 512;  /* set default size: 512 MB */
	} else
		limit_size = 0;

	if (sync_action == E_LOG_SEM_WITH_MT) {
		if (sem_name == NULL || *sem_name == 0) {
			sem_name = acl_concatenate("/tmp/", acl_safe_basename(filename), ".sem", NULL);
		} else
			sem_name = acl_mystrdup(sem_name);
	} else if (sem_name) {
		sem_name = NULL;
	}

	h_log->h_log = e_log_new2(h_log->filename, priority, action,
			flush, limit_size, facility_name, sync_action, sem_name);

	if (sem_name)
		acl_myfree(sem_name);

	if (logme) {
		/*
		char cmd[1024], buf[512];

		snprintf(buf, sizeof(buf), "filename=%s, priority=%d, action=%d, flush=%d, "
			"limit_size=%d, facility_name=%s, sync_action=%d, sem_name=%s",
			h_log->filename, priority, action,
			flush, limit_size, facility_name, sync_action, sem_name);
		snprintf(cmd, sizeof(cmd), "echo '%s, buf(%s)' >> /tmp/test1.log", acl_vstring_str(log_buf), buf);
		system(cmd);
		*/

		e_log2(h_log->h_log, "master_env: %s", acl_vstring_str(log_buf));
	}

	e_log2(h_log->h_log, "filename=%s, priority=%d, action=%d, flush=%d, "
		"limit_size=%d, facility_name=%s, sync_action=%d, sem_name=%s",
		h_log->filename, priority, action,
		flush, limit_size, facility_name, sync_action, sem_name);
		
	if (log_buf)
		acl_vstring_free(log_buf);

	return (0);
}

static void __log_close(void *ctx acl_unused)
{
}

static void __log_write(void *ctx, const char *fmt, va_list ap)
{
	LOG_WRAP *h_log = (LOG_WRAP *) ctx;
#if 0
	char  buf[1024], cmd[2048];

	vsnprintf(buf, sizeof(buf), fmt, ap);

	snprintf(cmd, sizeof(cmd), "echo \"%s\" >> /tmp/out.txt", buf);
	system(cmd);
#endif

#if 0
	LC_SysLogFmtWrite2(&h_log->h_log, fmt, ap);
#endif
	if (h_log->h_log)
		e_vlog2(h_log->h_log, fmt, ap);
}

void app_set_libcore_log(void)
{
	LOG_WRAP *h_log;

	h_log = acl_mycalloc(1, sizeof(LOG_WRAP));
	if (h_log == NULL)
		return;

	acl_msg_register(__log_open, __log_close, __log_write, (void *) h_log);
	__log_wrap = h_log;
}

void app_libcore_log_end(void)
{
	if (__log_wrap == NULL)
		return;
	if (__log_wrap->h_log == NULL)
		return;
	acl_msg_info("service exit now");
	e_log_free(__log_wrap->h_log);
	__log_wrap->h_log = NULL;
}
#else
void app_set_libcore_log()
{
	const char *myname = "app_set_libcore_log";

	acl_msg_fatal("%s: not supported", myname);
}
#endif

