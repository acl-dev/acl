#include "lib_acl.h"
#include <assert.h>
#ifdef ACL_MS_WINDOWS
#include <process.h>
typedef int pid_t;
#endif
#include "sys_patch.h"
#include "logger.h"

typedef struct LOG_WRAP {
	char  filename[1024];
	ACL_VSTREAM *h_log;
	ACL_AIO *aio;
	ACL_AQUEUE *logger_queue;
#ifdef HAS_ELIB
	E_LOG_T *h_log;
#endif
} LOG_WRAP;

static LOG_WRAP *__log_wrap = NULL;
static pid_t  __pid;

#ifdef HAS_ELIB
#include "e_config.h"
#include "elib.h"

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

	if (h_log->h_log)
		e_vlog2(h_log->h_log, fmt, ap);
}

static void elib_logger_set(void)
{
	LOG_WRAP *h_log;

	h_log = acl_mycalloc(1, sizeof(LOG_WRAP));
	if (h_log == NULL)
		return;

	acl_msg_register(__log_open, __log_close, __log_write, (void *) h_log);
	__log_wrap = h_log;
}

static void elib_logger_end(void)
{
	if (__log_wrap == NULL)
		return;
	if (__log_wrap->h_log == NULL)
		return;
	acl_msg_info("service exit now");
	e_log_free(__log_wrap->h_log);
	__log_wrap->h_log = NULL;
}
#endif

static int logger_open(const char *filename, void *ctx)
{
	LOG_WRAP *h_log = (LOG_WRAP *) ctx;
#ifdef  ACL_MS_WINDOWS
	int   flag = O_RDWR | O_CREAT | O_APPEND | O_BINARY;
#else
	int   flag = O_RDWR | O_CREAT | O_APPEND;
#endif
	int   mode = S_IREAD | S_IWRITE;

	if (filename == NULL || *filename == 0)
		return (-1);

	acl_snprintf(h_log->filename, sizeof(h_log->filename), "%s", filename);
	h_log->h_log = acl_vstream_fopen(h_log->filename, flag, mode, 1024);
	if (h_log->h_log == NULL)
		return (-1);

	__pid = getpid();
	return (0);
}

/* just for thread logger */

static int thread_logger_open(const char *filename, void *ctx)
{
	return (logger_open(filename, ctx));
}

static void thread_logger_close(void *ctx acl_unused)
{
}

static void thread_logger_write(void *ctx, const char *fmt, va_list ap)
{
	LOG_WRAP *h_log = (LOG_WRAP *) ctx;
	char *pbuf;

	if (h_log->h_log) {
		pbuf = acl_mymalloc(1024);
		vsnprintf(pbuf, 1024, fmt, ap);
		acl_aqueue_push(h_log->logger_queue, pbuf);
	}
}

static void *thread_logger_main(void *ctx)
{
	LOG_WRAP *h_log = (LOG_WRAP *) ctx;
	ACL_AQUEUE *logger_queue = h_log->logger_queue;
	static char pre_buf[256];
	static struct tm local_time;
	static time_t  now, last;
	static int  pre_len;
	char *pbuf;

	while (1) {
		pbuf = acl_aqueue_pop(logger_queue);
		assert(pbuf != NULL);
		if (h_log->h_log == NULL)
			continue;

		time(&now);
		if (now - last >= 1) {
			last = now;
			(void) localtime_r(&now, &local_time);
			pre_len = snprintf(pre_buf, sizeof(pre_buf),
				"\npid(%d) %d/%d/%d %d:%d:%d| ", __pid,
				local_time.tm_year + 1900, local_time.tm_mon + 1,
				local_time.tm_mday, local_time.tm_hour,
				local_time.tm_min, local_time.tm_sec);
		}
		acl_vstream_writen(h_log->h_log, pre_buf, pre_len);
		acl_vstream_writen(h_log->h_log, pbuf, strlen(pbuf));
		acl_myfree(pbuf);
	}

	return (NULL);
}

static void thread_logger_set(void)
{
	LOG_WRAP *h_log;
	acl_pthread_attr_t attr;
	acl_pthread_t tid;

	h_log = acl_mycalloc(1, sizeof(LOG_WRAP));
	if (h_log == NULL)
		return;

	acl_msg_register(thread_logger_open, thread_logger_close,
		thread_logger_write, (void*)h_log);
	__log_wrap = h_log;

	h_log->logger_queue = acl_aqueue_new();
	acl_pthread_attr_init(&attr);
	acl_pthread_create(&tid, &attr, thread_logger_main, h_log);
}

static void thread_logger_end(void)
{
}

/* just for buffer logger */

static int buffer_logger_open(const char *filename, void *ctx)
{
	return (logger_open(filename, ctx));
}

static void buffer_logger_close(void *ctx acl_unused)
{
}

static void buffer_logger_write(void *ctx, const char *fmt, va_list ap)
{
	LOG_WRAP *h_log = (LOG_WRAP *) ctx;
	static char buf[1024], pre_buf[256];
	static struct tm local_time;
	static time_t  now, last;
	static int  pre_len;
	int   n;

	if (h_log->h_log) {
		time(&now);
		if (now - last >= 1) {
			last = now;
			(void) localtime_r(&now, &local_time);
			pre_len = snprintf(pre_buf, sizeof(pre_buf),
				"\npid(%d) %d/%d/%d %d:%d:%d| ", __pid,
				local_time.tm_year + 1900, local_time.tm_mon + 1,
				local_time.tm_mday, local_time.tm_hour,
				local_time.tm_min, local_time.tm_sec);
		}
		acl_vstream_buffed_writen(h_log->h_log, pre_buf, pre_len);
		n = vsnprintf(buf, sizeof(buf), fmt, ap);
		acl_vstream_buffed_writen(h_log->h_log, buf, n);
	}
}

static void buffed_logger_fflush(int event_type acl_unused, void *context)
{
	LOG_WRAP *h_log = (LOG_WRAP*) context;

	if (h_log->h_log != NULL) {
		acl_vstream_fflush(h_log->h_log);
	}
}

static void buffed_logger_set(ACL_AIO *aio)
{
	LOG_WRAP *h_log;

	assert(aio != NULL);

	h_log = acl_mycalloc(1, sizeof(LOG_WRAP));
	if (h_log == NULL)
		return;
	h_log->aio = aio;

	acl_msg_register(buffer_logger_open, buffer_logger_close,
		buffer_logger_write, (void*)h_log);
	(void) acl_aio_request_timer(aio, buffed_logger_fflush, h_log, 1, 1);
	__log_wrap = h_log;
}

static void buffed_logger_end(void)
{
}

/* just for default logger */
static int default_logger_open(const char *filename, void *ctx)
{
	return (logger_open(filename, ctx));
}

static void default_logger_close(void *ctx acl_unused)
{
}

static void default_logger_write(void *ctx, const char *fmt, va_list ap)
{
	LOG_WRAP *h_log = (LOG_WRAP *) ctx;
	static char buf[1024], pre_buf[256];
	static struct tm local_time;
	static time_t  now, last;
	static int  pre_len;
	int   n;

	if (h_log->h_log) {
		time(&now);
		if (now - last >= 1) {
			last = now;
			(void) localtime_r(&now, &local_time);
			pre_len = snprintf(pre_buf, sizeof(pre_buf),
				"\npid(%d) %d/%d/%d %d:%d:%d| ", __pid,
				local_time.tm_year + 1900, local_time.tm_mon + 1,
				local_time.tm_mday, local_time.tm_hour,
				local_time.tm_min, local_time.tm_sec);
		}
		acl_vstream_writen(h_log->h_log, pre_buf, pre_len);
		n = vsnprintf(buf, sizeof(buf), fmt, ap);
		acl_vstream_writen(h_log->h_log, buf, n);
	}
}

static void default_logger_set(void)
{
	LOG_WRAP *h_log;

	h_log = acl_mycalloc(1, sizeof(LOG_WRAP));
	if (h_log == NULL)
		return;

	acl_msg_register(default_logger_open, default_logger_close,
		default_logger_write, (void*)h_log);
	__log_wrap = h_log;
}

static void default_logger_end(void)
{
}

/* logger main  */
static char __logger_name[256];

void logger_set(const char *logger_name, ACL_AIO *aio)
{
	const char *myname = "logger_set";

	ACL_SAFE_STRNCPY(__logger_name, logger_name, sizeof(__logger_name));
	if (strcasecmp(logger_name, "buffed_logger") == 0)
		buffed_logger_set(aio);
	else if (strcasecmp(logger_name, "default_logger") == 0)
		default_logger_set();
	else if (strcasecmp(logger_name, "thread_logger") == 0)
		thread_logger_set();
#ifdef HAS_ELIB
	else if (strcasecmp(logger_name, "elib_logger") == 0)
		elib_logger_set();
#endif
	else {
		printf("%s(%d): unknown logger name(%s)\r\n",
			myname, __LINE__, logger_name);
		return;
	}
}

void logger_end(void)
{
	if (strcasecmp(__logger_name, "buffed_logger") == 0)
		buffed_logger_end();
	else if (strcasecmp(__logger_name, "default_logger") == 0)
		default_logger_end();
	else if (strcasecmp(__logger_name, "thread_logger") == 0)
		thread_logger_end();
#ifdef HAS_ELIB
	else if (strcasecmp(__logger_name, "elib_logger") == 0)
		elib_logger_end();
#endif
}
