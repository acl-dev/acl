#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE
#include "stdlib/acl_define.h"
#include "stdlib/acl_mymalloc.h"

/* #include <time.h> */
#include <stdio.h>
#include <sys/stat.h>

#ifdef	ACL_WINDOWS
# include <process.h>
# ifdef	ACL_WINDOWS
/* #include <io.h> */
#  include <fcntl.h>
#  include <sys/stat.h>
/* #include <sys/types.h> */
# endif
#elif	defined(ACL_UNIX)
#include <stdlib.h>
#include <unistd.h>
#ifndef _GNU_SOURCE 
#define _GNU_SOURCE
#endif
#ifndef __USE_UNIX98
# define __USE_UNIX98
#endif
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#else
# error "unknown OS type"
#endif

#ifdef ACL_BCB_COMPILER
#pragma hdrstop
#endif

#include "stdlib/acl_sys_patch.h"
#include "stdlib/acl_iostuff.h"
#include "stdlib/acl_iterator.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_argv.h"
#include "net/acl_sane_inet.h"
#include "stdlib/acl_mylog.h"

#endif

#include "../private/private_fifo.h"
#include "../private/private_vstream.h"
#include "../private/thread.h"

struct SOCK_ADDR {
	union {
		struct sockaddr_storage ss;
#ifdef AF_INET6
		struct sockaddr_in6 in6;
#endif
		struct sockaddr_in in;
		struct sockaddr sa;
	} sa;
};

struct ACL_LOG {
	ACL_VSTREAM *fp;
	char *path;
	int   flag;
#define ACL_LOG_F_DEAD		(1 << 0)
#define	ACL_LOG_F_FIXED		(1 << 1)

	int   type;
#define ACL_LOG_T_UNKNOWN       0
#define ACL_LOG_T_FILE          1
#define ACL_LOG_T_TCP           2
#define ACL_LOG_T_UDP           3
#define ACL_LOG_T_UNIX          4

#define	IS_NET_STREAM(x)	((x)->type == ACL_LOG_T_TCP \
				|| (x)->type == ACL_LOG_T_UNIX)

	acl_pthread_mutex_t *lock;
	char   logpre[256];
	struct SOCK_ADDR from;
	struct SOCK_ADDR dest;
	int    from_len;
	time_t last_open;		/**< 上次日志打开时间 */
	time_t reopen_inter;		/**< 日志重新打开的最小时间间隔 */
	acl_uint64   count;		/**< 已经记录的日志条数 */
};

#ifdef ACL_WINDOWS
# define strdup _strdup
#endif

static int  __log_thread_id = 0;
static ACL_FIFO *__loggers = NULL;

static int __log_close_onexec = 1;

void acl_log_close_onexec(int yes)
{
	__log_close_onexec = yes;
}

void acl_log_add_tid(int onoff)
{
	__log_thread_id = onoff ? 1 : 0;
}

static void init_log_mutex(acl_pthread_mutex_t *lock)
{
#ifdef ACL_UNIX
	int n1, n2;
	pthread_mutexattr_t attr;

	n1 = pthread_mutexattr_init(&attr);

	/* 使用了 pthread_atfork() 来避免 fork 后的死锁，因为在 fork 前调用过
	 * 加锁过程，所以需将此锁设为递归锁 --- zsx, 2019.8.6
	 */
# if defined(ACL_FREEBSD) || defined(ACL_SUNOS5) || defined(ACL_MACOSX)
	n2 = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
# elif defined(MINGW)
	n2 = 0
# else
	n2 = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
# endif
	thread_mutex_init(lock, !n1 && !n2 ? &attr : NULL);
#else
	thread_mutex_init(lock, NULL);
#endif
}

void acl_log_fp_set(ACL_VSTREAM *fp, const char *logpre)
{
	const char *myname = "acl_log_fp_set";
	ACL_LOG *log;
	ACL_ITER iter;

	acl_assert(fp);

	if (__loggers == NULL) {
		__loggers = private_fifo_new();
	}

	acl_foreach(iter, __loggers) {
		log = (ACL_LOG*) iter.data;
		if (strcmp(log->path, ACL_VSTREAM_PATH(fp)) == 0) {
			acl_msg_warn("%s(%d): log %s has been opened.",
				myname, __LINE__, log->path);
			return;
		}
	}

#ifdef	ACL_UNIX
	if (__log_close_onexec) {
		acl_close_on_exec(ACL_VSTREAM_SOCK(fp), ACL_CLOSE_ON_EXEC);
	}
#endif
	log = (ACL_LOG*) calloc(1, sizeof(ACL_LOG));
	log->fp = fp;
	log->path = strdup(ACL_VSTREAM_PATH(fp));
	log->type = ACL_LOG_T_UNKNOWN;
	log->lock = (acl_pthread_mutex_t*)
		calloc(1, sizeof(acl_pthread_mutex_t));
	init_log_mutex(log->lock);
	if (logpre && *logpre) {
		snprintf(log->logpre, sizeof(log->logpre), "%s", logpre);
	} else {
		log->logpre[0] = 0;
	}
	log->flag |= ACL_LOG_F_FIXED;
	if (fp->type & ACL_VSTREAM_TYPE_FILE) {
		log->type = ACL_LOG_T_FILE;
	} else if (fp->type & ACL_VSTREAM_TYPE_LISTEN_INET) {
		log->type= ACL_LOG_T_UDP;
	}
	private_fifo_push(__loggers, log);
}

static int open_file_log(const char *filename, const char *logpre)
{
	const char *myname = "open_file_log";
#ifdef	ACL_WINDOWS
	int   flag = O_RDWR | O_CREAT | O_APPEND | O_BINARY;
#else
	int   flag = O_RDWR | O_CREAT | O_APPEND;
#endif
#ifdef ACL_ANDROID
	int   mode = 0644;
#else
	int   mode = S_IREAD | S_IWRITE;
#endif
	ACL_LOG *log;
	ACL_ITER iter;
	ACL_VSTREAM *fp;
	ACL_FILE_HANDLE fh;

	acl_foreach(iter, __loggers) {
		log = (ACL_LOG*) iter.data;
		if (strcmp(log->path, filename) == 0) {
			acl_msg_warn("%s(%d): log %s has been opened.",
				myname, __LINE__, filename);
			return 0;
		}
	}

	fh = acl_file_open(filename, flag, mode);
	if (fh == ACL_FILE_INVALID) {
		printf("%s(%d): open %s error(%s)", myname, __LINE__,
			filename, acl_last_serror());
		return -1;
	}

	fp = private_vstream_fhopen(fh, O_RDWR);

#if 0
	acl_vstream_set_path(fp, filename);
#else
	fp->path = strdup(filename);
#endif

#ifdef	ACL_UNIX
	if (__log_close_onexec) {
		acl_close_on_exec(fh, ACL_CLOSE_ON_EXEC);
	}
#endif

	log = (ACL_LOG*) calloc(1, sizeof(ACL_LOG));
	log->last_open = time(NULL);
	log->fp = fp;
	log->path = strdup(filename);
	log->type = ACL_LOG_T_FILE;
	log->lock = (acl_pthread_mutex_t*)
		calloc(1, sizeof(acl_pthread_mutex_t));
	init_log_mutex(log->lock);
	if (logpre && *logpre) {
		snprintf(log->logpre, sizeof(log->logpre), "%s", logpre);
	} else {
		log->logpre[0] = 0;
	}

	private_fifo_push(__loggers, log);
	return 0;
}

static int reopen_log(ACL_LOG *log)
{
	time_t now = time(NULL);

#undef	RETURN
#define	RETURN(x) do {  \
	if (log->lock) {  \
		thread_mutex_unlock(log->lock);  \
	} \
	return (x);  \
} while (0)

	if (log->lock) {
		thread_mutex_lock(log->lock);
	}

	if (!(log->flag & ACL_LOG_F_DEAD)
		|| (log->flag & ACL_LOG_F_FIXED)
		|| !IS_NET_STREAM(log)
		|| log->fp == NULL
		|| log->reopen_inter <= 0) {

		RETURN(-1);
	}

	if (log->count == 0) {
		if (now - log->last_open < 5 * log->reopen_inter) {
			RETURN(-1);
		}
	} else if (now - log->last_open < log->reopen_inter) {
		RETURN(-1);
	}

	if (log->fp->path) {
		free(log->fp->path);
		log->fp->path = NULL;
	}
#if 0
	if (log->fp->addr_local) {
		free(log->fp->addr_local);
		log->fp->addr_local = NULL;
	}
	if (log->fp->addr_peer) {
		free(log->fp->addr_peer);
		log->fp->addr_peer = NULL;
	}
#endif

	private_vstream_close(log->fp);
	acl_assert(log->path);
	log->fp = private_vstream_connect(log->path, 60, 60);
	log->last_open = time(NULL);
	if (log->fp == NULL) {
		RETURN(-1);
	}
	log->flag &= ~ACL_LOG_F_DEAD;
	log->last_open = time(NULL);
	RETURN(0);
}

static int open_stream_log(const char *addr, const char *logpre, int type)
{
	const char *myname = "open_stream_log";
	ACL_VSTREAM *fp;
	ACL_LOG *log;
	ACL_ITER iter;

	acl_foreach(iter, __loggers) {
		log = (ACL_LOG*) iter.data;
		if (strcmp(log->path, addr) == 0 && log->type == type) {
			acl_msg_warn("%s(%d): log(%s) has been opened!",
				myname, __LINE__, addr);
			return 0;
		}
	}

	fp = private_vstream_connect(addr, 60, 60);
	if (fp == NULL) {
		printf("%s(%d): connect %s error(%s)\n",
			myname, __LINE__, addr, acl_last_serror());
		return -1;
	}

	log = (ACL_LOG*) calloc(1, sizeof(ACL_LOG));
	log->last_open = time(NULL);
	log->reopen_inter = 60;
	log->fp = fp;
	log->path = strdup(addr);
	log->lock = (acl_pthread_mutex_t*)
		calloc(1, sizeof(acl_pthread_mutex_t));
	thread_mutex_init(log->lock, NULL);
	log->type = type;
	if (logpre && *logpre) {
		snprintf(log->logpre, sizeof(log->logpre), "%s", logpre);
	} else {
		log->logpre[0] = 0;
	}

	private_fifo_push(__loggers, log);
	return 0;
}

static int open_tcp_log(const char *addr, const char *logpre)
{
	return open_stream_log(addr, logpre, ACL_LOG_T_TCP);
}

static int open_unix_log(const char *addr, const char *logpre)
{
	return open_stream_log(addr, logpre, ACL_LOG_T_UNIX);
}

#ifdef ACL_WINDOWS
# define len_t int
# define ptr_t void
#elif defined(ACL_UNIX)
# define len_t socklen_t
# define ptr_t char
#else
# error "unknow os"
#endif

static int udp_read(ACL_SOCKET fd, void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *fp acl_unused, void *arg)
{
	ACL_LOG *log = (ACL_LOG*) arg;
	struct sockaddr *sa = &log->from.sa.sa;
#ifdef ACL_WINDOWS
	return (int) recvfrom(fd, buf, (int) size, 0, sa, (len_t*) &log->from_len);
#else
	return (int) recvfrom(fd, buf, size, 0, sa, (len_t*) &log->from_len);
#endif
}

static int udp_write(ACL_SOCKET fd, const void *buf, size_t size,
	int timeout acl_unused, ACL_VSTREAM *fp acl_unused, void *arg)
{
	ACL_LOG *log = (ACL_LOG*) arg;
	struct sockaddr *sa = &log->dest.sa.sa;

#ifdef ACL_WINDOWS
	return (int)sendto(fd, (const ptr_t*)buf, (int) size, 0,
		sa, (len_t) sizeof(log->dest));
#else
	return (int) sendto(fd, (const ptr_t*) buf, size, 0,
			sa, (len_t) sizeof(log->dest));
#endif
}

static int open_udp_log(const char *addr, const char *logpre)
{
	const char *myname = "open_udp_log";
	ACL_LOG *log;
	ACL_ITER iter;
	ACL_SOCKET fd;
	char  ip[128], *ptr, *sport;
	int   err, port;
	struct addrinfo hints, *res0, *res;

	snprintf(ip, sizeof(ip), "%s", addr);
	ptr = strchr(ip, ':');
	if (ptr == NULL || *(ptr + 1) == 0) {
		printf("invalid addr: %s\r\n", addr);
		abort();
	}

	*ptr++ = 0;
	sport  = ptr;
	port   = atoi(sport);
	if (sport == NULL) {
		printf("invalid addr: %s, port: %d\r\n", addr, port);
		abort();
	}

	acl_foreach(iter, __loggers) {
		log = (ACL_LOG*) iter.data;
		if (!strcmp(log->path, addr) && log->type == ACL_LOG_T_UDP) {
			acl_msg_warn("%s(%d): log(%s) has been opened!",
				myname, __LINE__, addr);
			return 0;
		}
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family   = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
#ifdef	ACL_MACOSX
	hints.ai_flags    = AI_DEFAULT;
#elif	defined(ACL_ANDROID)
	hints.ai_flags    = AI_ADDRCONFIG;
#elif defined(ACL_WINDOWS)
	hints.ai_protocol = IPPROTO_UDP;
# if _MSC_VER >= 1500
	hints.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;
# endif
#else
	hints.ai_flags    = AI_V4MAPPED | AI_ADDRCONFIG;
#endif

	if ((err = getaddrinfo(ip, sport, &hints, &res0))) {
		printf("%s(%d), %s: getaddrinfo error %s, peer=%s",
			__FILE__, __LINE__, myname, gai_strerror(err), ip);
		abort();
	}

	fd = ACL_SOCKET_INVALID;

	for (res = res0; res != NULL; res = res->ai_next) {
		fd = socket(res->ai_family, res->ai_socktype,
				res->ai_protocol);
		if (fd != ACL_SOCKET_INVALID) {
			break;
		}

		printf("%s: socket %s", myname, acl_last_serror());
	}

	if (fd == ACL_SOCKET_INVALID || res == NULL) {
		printf("%s(%d), %s: invalid socket, addr: %s\r\n",
			__FILE__, __LINE__, myname, addr);
		abort();
	}

	log = (ACL_LOG*) calloc(1, sizeof(ACL_LOG));

	log->fp = private_vstream_fdopen(fd, O_RDWR, 1024, 0,
			ACL_VSTREAM_TYPE_SOCK);
	private_vstream_ctl(log->fp,
			ACL_VSTREAM_CTL_READ_FN, udp_read,
			ACL_VSTREAM_CTL_WRITE_FN, udp_write,
			ACL_VSTREAM_CTL_CONTEXT, log,
			ACL_VSTREAM_CTL_END);

	log->last_open = time(NULL);
	log->dest.sa.sa.sa_family = res->ai_family;

	if (res->ai_family == AF_INET) {
		log->dest.sa.in.sin_addr.s_addr = inet_addr(ip);
		log->dest.sa.in.sin_port = htons(port);
	}
#ifdef AF_INET6
	else if (res->ai_family == AF_INET6) {
		log->dest.sa.in6.sin6_port = htons(port);
		if (inet_pton(res->ai_family, ip,
			&log->dest.sa.in6.sin6_addr) != 1) {

			printf("%s(%d), %s: inet_pton error: %s, ip: %s\r\n",
				__FILE__, __LINE__, myname,
				acl_last_serror(), ip);
			abort();
		}
	}
#endif
	else {
		printf("%s(%d), %s: invalid sa_family: %d, ip: %s\r\n",
			__FILE__, __LINE__, myname, res->ai_family, ip);
		abort();
	}

	log->from_len = sizeof(log->from);
	log->path = strdup(addr);
	log->type = ACL_LOG_T_UDP;
	log->lock = (acl_pthread_mutex_t*)
		calloc(1, sizeof(acl_pthread_mutex_t));
	thread_mutex_init(log->lock, NULL);

	if (logpre && *logpre) {
		snprintf(log->logpre, sizeof(log->logpre), "%s", logpre);
	} else {
		log->logpre[0] = 0;
	}

	private_fifo_push(__loggers, log);
	freeaddrinfo(res0);

	return 0;
}

/*
 * recipient:
 *  tcp:127.0.0.1:8088
 *  udp:127.0.0.1:8088
 *  unix:/var/log/unix.sock
 *  file:/var/log/unix.log
 *  /var/log/unix.log
 */
static int open_log(const char *recipient, const char *logpre)
{
	const char *myname = "open_log";
	const char *ptr;

	if (strncasecmp(recipient, "tcp:", 4) == 0) {
		ptr = recipient + 4;
		if (acl_ipv4_addr_valid(ptr) == 0) {
			printf("%s(%d): recipient(%s) invalid",
				myname, __LINE__, recipient);
			return -1;
		}
		return open_tcp_log(ptr, logpre);
	} else if (strncasecmp(recipient, "udp:", 4) == 0) {
		ptr = recipient + 4;
		if (acl_ipv4_addr_valid(ptr) == 0) {
			printf("%s(%d): recipient(%s) invalid",
				myname, __LINE__, recipient);
			return -1;
		}
		return open_udp_log(ptr, logpre);
	} else if (strncasecmp(recipient, "unix:", 5) == 0) {
		ptr = recipient + 5;
		if (*ptr == 0) {
			printf("%s(%d): recipient(%s) invalid",
				myname, __LINE__, recipient);
			return -1;
		}
		return open_unix_log(ptr, logpre);
	} else if (strncasecmp(recipient, "file:", 5) == 0) {
		ptr = recipient + 5;
		if (*ptr == 0) {
			printf("%s(%d): recipient(%s) invalid",
				myname, __LINE__, recipient);
			return -1;
		}
		return open_file_log(recipient, logpre);
	} else
		return open_file_log(recipient, logpre);
}

#if defined(ACL_UNIX) && !defined(ACL_ANDROID)
static void fork_prepare(void)
{
	if (__loggers != NULL) {
		ACL_ITER iter;
		acl_foreach(iter, __loggers) {
			ACL_LOG *log = (ACL_LOG *) iter.data;
			if (log->lock) {
				thread_mutex_unlock(log->lock);
				thread_mutex_lock(log->lock);
			}
		}
	}
}

static void fork_in_parent(void)
{
	if (__loggers != NULL) {
		ACL_ITER iter;
		acl_foreach(iter, __loggers) {
			ACL_LOG *log = (ACL_LOG *) iter.data;
			if (log->lock) {
				thread_mutex_unlock(log->lock);
			}
		}
	}
}

static void fork_in_child(void)
{
	if (__loggers != NULL) {
		ACL_ITER iter;
		acl_foreach(iter, __loggers) {
			ACL_LOG *log = (ACL_LOG *) iter.data;
			if (log->lock) {
				thread_mutex_unlock(log->lock);
				/* init_log_mutex(log->lock); */
			}
		}
	}
}
#endif

/*
 * recipients 可以是以下日志格式的组合:
 *  tcp:127.0.0.1:8088
 *  udp:127.0.0.1:8088
 *  unix:/var/log/unix.sock
 *  file:/var/log/unix.log
 *  /var/log/unix.log
 * 如：tcp:127.0.0.1:8088|/var/log/unix.log
 */
int acl_open_log(const char *recipients, const char *logpre)
{
	const char *myname = "acl_open_log";
	const char *ptr;
	ACL_ARGV *argv = NULL;
	ACL_ITER  iter;

	if (recipients == NULL || *recipients == 0) {
		printf("%s(%d): recipients null\n", myname, __LINE__);
		return -1;
	} else if (logpre == NULL || *logpre == 0) {
		printf("%s(%d): logpre null\n", myname, __LINE__);
		return -1;
	}

	if (__loggers == NULL) {
		__loggers = private_fifo_new();
	}

	argv = acl_argv_split(recipients, "|");
	acl_foreach(iter, argv) {
		ptr = (const char*) iter.data;
		if (open_log(ptr, logpre) < 0) {
			acl_argv_free(argv);
			return -1;
		}
	}

	acl_argv_free(argv);

#if defined(ACL_UNIX) && !defined(ACL_ANDROID)
	pthread_atfork(fork_prepare, fork_in_parent, fork_in_child);
#endif
	return 0;
}

void acl_logtime_fmt(char *buf, size_t size)
{
	time_t	now;
#ifdef	ACL_UNIX
	struct tm local_time;

	(void) time (&now);
	(void) localtime_r(&now, &local_time);
	strftime(buf, size, "%Y/%m/%d %H:%M:%S", &local_time);
#elif	defined(ACL_WINDOWS)
	struct tm *local_time;

	(void) time (&now);
	local_time = localtime(&now);
	strftime(buf, size, "%Y/%m/%d %H:%M:%S", local_time);
#else
# error "unknown OS type"
#endif
}

#ifdef	ACL_UNIX

static char *get_buf(const char *prefix, const char *fmt, va_list ap,
	const char *suffix, size_t *len)
{
	va_list ap_tmp;
	char  *buf, *ptr;
	size_t prefix_len = strlen(prefix);
	size_t suffix_len = suffix ? strlen(suffix) : 0;
	size_t total_len, left_len;
	int    ret;

	total_len = prefix_len + 1024;
	buf = (char*) malloc(total_len);
	acl_assert(buf);

	strcpy(buf, prefix);
	ptr = buf + prefix_len;
	left_len = total_len - prefix_len;
	va_copy(ap_tmp, ap);
	ret = vsnprintf(ptr, left_len, fmt, ap_tmp);
	acl_assert(ret > 0);

	*len = prefix_len + ret;

	if (ret >= (int) left_len) {
		total_len = prefix_len + ret + 1;
		buf = (char*) realloc(buf, total_len);
		acl_assert(buf);

		ptr = buf + prefix_len;
		left_len = total_len - prefix_len;
		va_copy(ap_tmp, ap);
		ret = vsnprintf(ptr, left_len, fmt, ap_tmp);
		acl_assert(ret > 0 && ret < (int) left_len);
	}

	if (suffix_len == 0) {
		return buf;
	}

	*len = *len + suffix_len;
	if (*len >= total_len) {
		total_len = *len + 1;
		buf = (char*) realloc(buf, total_len);
		acl_assert(buf);
	}

	ptr = buf + prefix_len + ret;
	memcpy(ptr, suffix, suffix_len);
	ptr[suffix_len] = 0;

	return buf;
}

#else

char *get_buf(const char *prefix, const char *fmt, va_list ap,
	const char *suffix, size_t *len)
{
	char  *buf, *ptr;
	size_t prefix_len = strlen(prefix);
	size_t suffix_len = suffix ? strlen(suffix) : 0;
	size_t total_len, left_len;
	int    ret;

	total_len = prefix_len + 1024;
	buf = (char*) malloc(total_len);
	acl_assert(buf);

	strcpy(buf, prefix);
	ptr = buf + prefix_len;
	left_len = total_len - prefix_len;
	ret = vsnprintf(ptr, left_len, fmt, ap);
	if (ret > 0 && ret < (int) left_len) {
		*len = prefix_len + ret;
	} else {
		int i = 0;
		for (;;) {
			total_len += 1024;
			buf = (char*) realloc(buf, total_len);
			acl_assert(buf);

			ptr = buf + prefix_len;
			left_len = total_len - prefix_len;
			ret = vsnprintf(ptr, left_len, fmt, ap);
			if (ret > 0 && ret < (int) left_len) {
				*len = prefix_len + ret;
				break;
			}
			if (++i >= 10000) {
				abort();
			}
		}
	}

	if (suffix_len == 0) {
		return buf;
	}

	*len = *len + suffix_len;
	if (*len >= total_len) {
		total_len = *len + 1;
		buf = (char*) realloc(buf, total_len);
		acl_assert(buf);
	}

	ptr = buf + prefix_len + ret;
	memcpy(ptr, suffix, suffix_len);
	ptr[suffix_len] = 0;

	return buf;
}

#endif

static void file_vsyslog(ACL_LOG *log, const char *info,
	const char *fmt, va_list ap)
{
	char   fmtstr[128], tbuf[1024], *buf;
	size_t len;

	acl_logtime_fmt(fmtstr, sizeof(fmtstr));

	if (__log_thread_id) {
#ifdef MINGW
		snprintf(tbuf, sizeof(tbuf), "%s %s (pid=%d, tid=%u)(%s): ",
			fmtstr, log->logpre, (int) getpid(),
			(unsigned int) acl_pthread_self(), info);
#elif defined(ACL_LINUX)
		snprintf(tbuf, sizeof(tbuf), "%s %s (pid=%d, tid=%llu)(%s): ",
			fmtstr, log->logpre, (int) getpid(),
			(unsigned long long int) acl_pthread_self(), info);
#elif defined(SUNOS5)
		snprintf(tbuf, sizeof(tbuf), "%s %s (pid=%d, tid=%d)(%s): ",
			fmtstr, log->logpre, (int) getpid(),
			(int) acl_pthread_self(), info);
#elif defined(ACL_WINDOWS)
		snprintf(tbuf, sizeof(tbuf), "%s %s (pid=%d, tid=%d)(%s): ",
			fmtstr, log->logpre, (int) _getpid(),
			(int) acl_pthread_self(), info);
#else
		snprintf(tbuf, sizeof(tbuf), "%s %s: (%s): ",
			fmtstr, log->logpre, info);
#endif
	} else {
#if defined(SUNOS5)
		snprintf(tbuf, sizeof(tbuf), "%s %s (pid=%d)(%s): ",
			fmtstr, log->logpre, (int) getpid(), info);
#elif defined(ACL_WINDOWS)
		snprintf(tbuf, sizeof(tbuf), "%s %s (pid=%d)(%s): ",
			fmtstr, log->logpre, (int) _getpid(), info);
#else
		snprintf(tbuf, sizeof(tbuf), "%s %s (pid=%d)(%s): ",
			fmtstr, log->logpre, (int) getpid(), info);
#endif
	}

	buf = get_buf(tbuf, fmt, ap, "\r\n", &len);

	if (private_vstream_writen(log->fp, buf, len) == ACL_VSTREAM_EOF) {
		log->flag |= ACL_LOG_F_DEAD;
	} else {
		log->count++;
	}

	free(buf);
}

static void net_vsyslog(ACL_LOG *log, const char *info,
	const char *fmt, va_list ap)
{
	char  tbuf[1024], *buf;
	size_t len;

	if (__log_thread_id) {
#ifdef MINGW
		snprintf(tbuf, sizeof(tbuf), " %s (pid=%d, tid=%u)(%s): ",
			log->logpre, (int) getpid(),
			(unsigned int) acl_pthread_self(), info);
#elif defined(ACL_LINUX)
		snprintf(tbuf, sizeof(tbuf), " %s (pid=%d, tid=%llu)(%s): ",
			log->logpre, (int) getpid(),
			(unsigned long long int) acl_pthread_self(), info);
#elif defined(SUNOS5)
		snprintf(tbuf, sizeof(tbuf), " %s (pid=%d, tid=%d)(%s): ",
			log->logpre, (int) getpid(),
			(int) acl_pthread_self(), info);
#elif defined(ACL_WINDOWS)
		snprintf(tbuf, sizeof(tbuf), " %s (pid=%d, tid=%d)(%s): ",
			log->logpre, (int) _getpid(),
			(int) acl_pthread_self(), info);
#else
		snprintf(tbuf, sizeof(tbuf), " %s: (%s): ", log->logpre, info);
#endif
	} else {
#if defined(SUNOS5)
		snprintf(tbuf, sizeof(tbuf), " %s (pid=%d)(%s): ",
			log->logpre, (int) getpid(), info);
#elif defined(ACL_WINDOWS)
		snprintf(tbuf, sizeof(tbuf), " %s (pid=%d)(%s): ",
			log->logpre, (int) _getpid(), info);
#else
		snprintf(tbuf, sizeof(tbuf), " %s (pid=%d)(%s): ",
			log->logpre, (int) getpid(), info);
#endif
	}

	buf = get_buf(tbuf, fmt, ap, log->type == ACL_LOG_T_UDP
		? NULL : "\r\n", &len);

	if (log->lock && log->type != ACL_LOG_T_UDP) {
		thread_mutex_lock(log->lock);
	}

	if (log->type == ACL_LOG_T_UDP) {
		(void) private_vstream_write(log->fp, buf, len);
		log->count++;
	} else if (private_vstream_writen(log->fp, buf, len) == ACL_VSTREAM_EOF) {
		log->flag |= ACL_LOG_F_DEAD;
	} else {
		log->count++;
	}

	if (log->lock && log->type != ACL_LOG_T_UDP) {
		thread_mutex_unlock(log->lock);
	}

	free(buf);
}

int acl_write_to_log2(const char *info, const char *fmt, va_list ap)
{
	ACL_ITER iter;
	ACL_LOG *log;
#ifdef ACL_UNIX
	va_list  tmp;
#endif

	if (__loggers == NULL) {
		return 0;
	}

#ifdef ACL_UNIX
	acl_foreach(iter, __loggers) {
		log = (ACL_LOG*) iter.data;
		if ((log->flag & ACL_LOG_F_DEAD)) {
			if (reopen_log(log) < 0) {
				continue;
			}
		}
		va_copy(tmp, ap);
		if (log->type == ACL_LOG_T_FILE) {
			file_vsyslog(log, info, fmt, tmp);
		} else if (log->type == ACL_LOG_T_TCP
			|| log->type == ACL_LOG_T_UDP
			|| log->type == ACL_LOG_T_UNIX) {

			net_vsyslog(log, info, fmt, tmp);
		}
	}
#else
	acl_foreach(iter, __loggers) {
		log = (ACL_LOG*) iter.data;
		if ((log->flag & ACL_LOG_F_DEAD)) {
			if (reopen_log(log) < 0) {
				continue;
			}
		}
		if (log->type == ACL_LOG_T_FILE) {
			file_vsyslog(log, info, fmt, ap);
		} else if (log->type == ACL_LOG_T_TCP
			|| log->type == ACL_LOG_T_UDP
			|| log->type == ACL_LOG_T_UNIX) {

			net_vsyslog(log, info, fmt, ap);
		}
	}
#endif

	return 0;
}

int acl_write_to_log(const char *fmt, ...)
{
	va_list	ap;
	int   ret;

	va_start (ap, fmt);
	ret = acl_write_to_log2("-", fmt, ap);
	va_end(ap);
	return ret;
}

void acl_close_log()
{
	ACL_LOG *log;

	if (__loggers == NULL) {
		return;
	}

	while (1) {
		log = (ACL_LOG*) private_fifo_pop(__loggers);
		if (log == NULL) {
			break;
		}

		if ((log->flag & ACL_LOG_F_FIXED)) {
			continue;
		}

		if (log->fp) {
			if (log->fp->path) {
				free(log->fp->path);
				log->fp->path = NULL;
			}
#if 0
			if (log->fp->addr_local) {
				free(log->fp->addr_local);
				log->fp->addr_local = NULL;
			}
			if (log->fp->addr_peer) {
				free(log->fp->addr_peer);
				log->fp->addr_peer = NULL;
			}
#endif
			private_vstream_close(log->fp);
		}

		if (log->path) {
			free(log->path);
		}
		if (log->lock) {
			thread_mutex_destroy(log->lock);
		}
		free(log);
	}

	free(__loggers);
	__loggers = NULL;
}
