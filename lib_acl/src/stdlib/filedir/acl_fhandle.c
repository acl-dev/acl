#include "StdAfx.h"
#ifndef ACL_PREPARE_COMPILE

#include "stdlib/acl_define.h"
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h> /* for S_IREAD */

#include "stdlib/acl_debug.h"
#include "stdlib/acl_msg.h"
#include "stdlib/acl_htable.h"
#include "stdlib/acl_mymalloc.h"
#include "stdlib/acl_mystring.h"
#include "stdlib/acl_sane_basename.h"
#include "stdlib/acl_make_dirs.h"
#include "stdlib/acl_myflock.h"
#include "stdlib/acl_fhandle.h"

# ifdef	ACL_UNIX
#  include <pthread.h>
extern int pthread_mutexattr_settype (pthread_mutexattr_t *__attr, int __kind);
# endif
#else
# ifdef ACL_UNIX
#  include <pthread.h>
extern int pthread_mutexattr_settype (pthread_mutexattr_t *__attr, int __kind);
# endif
#endif  /* ACL_PREPARE_COMPILE */

#define	PATH	ACL_VSTREAM_PATH
#define	STR	acl_vstring_str

static int __cache_max_size = 100;
static int __debug_section = 0;
static int __flags = 0;
static ACL_RING __fhandle_free_list;
static ACL_HTABLE *__fhandle_table = NULL;
static acl_pthread_mutex_t __fhandle_mutex;

#define	MUTEX_LOCK(mutex)	acl_thread_mutex_lock((mutex))
#define	MUTEX_UNLOCK(mutex)	acl_thread_mutex_unlock((mutex))

#if 1

/* 可嵌套式加锁 */

#define	LOCK_FS	do {  \
	int  __ret;  \
	if ((__flags & ACL_FHANDLE_F_LOCK ) != 0 && (__ret = MUTEX_LOCK(&__fhandle_mutex)) != 0)  \
		acl_msg_fatal("%s: lock fs error(%d)", myname, __ret);  \
} while (0)

#define	UNLOCK_FS do {  \
	int  __ret;  \
	if ((__flags & ACL_FHANDLE_F_LOCK ) != 0 && (__ret = MUTEX_UNLOCK(&__fhandle_mutex)) != 0)  \
		acl_msg_fatal("%s: unlock fs error(%d)", myname, __ret);  \
} while (0)

#else
#define	LOCK_FS
#define	UNLOCK_FS
#endif

static ACL_FHANDLE *__fhandle_alloc(size_t size, unsigned int oflags)
{
	const char *myname = "__fhandle_alloc";
	ACL_FHANDLE *fs;

	if (size < sizeof(ACL_FHANDLE))
		acl_msg_fatal("%s(%d): size(%d) < ACL_FHANDLE's size(%d)",
			myname, __LINE__, (int) size, (int) sizeof(ACL_FHANDLE));

	fs = (ACL_FHANDLE *) acl_mycalloc(1, size);
	fs->tid = acl_pthread_self();
	fs->size = size;
	fs->oflags = oflags;
	acl_ring_init(&fs->ring);
	if ((oflags & ACL_FHANDLE_O_MLOCK) != 0) {
#ifdef	ACL_UNIX
		acl_pthread_mutexattr_t attr;

		pthread_mutexattr_init(&attr);
# if	defined(ACL_FREEBSD) || defined(ACL_SUNOS5) || defined(ACL_MACOSX)
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
# elif	defined(MINGW)
		pthread_mutex_init(&fs->mutex, &attr);
# else
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
# endif
		pthread_mutex_init(&fs->mutex, &attr);
#else
		acl_pthread_mutex_init(&fs->mutex, NULL);
#endif
	}

	return (fs);
}

static void __fhandle_free(ACL_FHANDLE *fs)
{
	if (fs->fp)
		acl_vstream_fclose(fs->fp);  /* 文件锁自动释放 */
	if ((fs->oflags & ACL_FHANDLE_O_MLOCK) != 0)
		acl_pthread_mutex_destroy(&fs->mutex);
	acl_myfree(fs);
}

#include <stdio.h>

ACL_FHANDLE *acl_fhandle_open(size_t size, unsigned int oflags,
	const char *file_path,
	int (*on_open)(ACL_FHANDLE*, void*), void *open_arg,
	void (*on_close)(ACL_FHANDLE*))
{
	const char *myname = "acl_fhandle_open";
	char *ptr;
	ACL_FHANDLE *fs;
	unsigned int fopen_flags = O_RDWR | O_CREAT;

	LOCK_FS;

	/* 先查询缓存中是否存在 */
	fs = (ACL_FHANDLE *) acl_htable_find(__fhandle_table, file_path);
	if (fs) {
		fs->nrefer++; /* 增加引用计数，以防止被多次引用时提前释放 */
		/* 如果该缓存对象处于延迟关闭队列中，则需要从中删除 */
		if (fs->when_free > 0) {
			/* 从延迟关闭缓冲中去除 */
			acl_ring_detach(&fs->ring);
			fs->when_free = 0;
		}

		UNLOCK_FS;

		/* 是否打开后自动加锁 */
		if ((oflags & ACL_FHANDLE_O_EXCL) != 0)
			acl_fhandle_lock(fs);

		return (fs);
	}

	/* sanity check */
	size = size < sizeof(ACL_FHANDLE) ? sizeof(ACL_FHANDLE) : size;

	/* 分配新的缓存对象 */
	fs = __fhandle_alloc(size, oflags);
	fs->on_close = on_close;

	/* 是否自动检查并创建不存在的目录 */
	if ((oflags & ACL_FHANDLE_O_MKDIR) != 0) {
		ACL_VSTRING *tmpbuf = acl_vstring_alloc(256);
		/* 必须保证目录存在，如果不存在则强行创建该目录 */
		ptr = acl_sane_dirname(tmpbuf, file_path);
		if (ptr && strcmp(ptr, ".") != 0)
			acl_make_dirs(ptr, 0700);
		acl_vstring_free(tmpbuf);
	}

#ifdef	ACL_UNUX
	if ((oflags & ACL_FHANDLE_O_NOATIME) != 0)
		fopen_flags |= O_NOATIME;
	if ((oflags & ACL_FHANDLE_O_DIRECT) != 0)
		fopen_flags |= O_DIRECT;
	if ((oflags & ACL_FHANDLE_O_SYNC) != 0)
		fopen_flags |= O_SYNC;
#endif

	/* 创建新文件 */
	fs->fp = acl_vstream_fopen(file_path, fopen_flags, 0600, 4096);
	if (fs->fp == NULL) {
		UNLOCK_FS;
		acl_msg_error("%s(%d): fopen %s error(%s)", myname, __LINE__,
			file_path, acl_last_serror());
		__fhandle_free(fs);
		return (NULL);
	}

	fs->fsize = acl_vstream_fsize(fs->fp);
	if (fs->fsize == -1) {
		UNLOCK_FS;
		acl_msg_error("%s(%d): get %s size error(%s)",
			myname, __LINE__, PATH(fs->fp), acl_last_serror());
		__fhandle_free(fs);
		return (NULL);
	} else if (on_open) {
		/* maybe first open, need init the store */
		if (on_open(fs, open_arg) < 0) {
			acl_msg_warn("%s(%d): on_create return < 0 for %s",
				myname, __LINE__, PATH(fs->fp));
			UNLOCK_FS;
			__fhandle_free(fs);
			return (NULL);
		}
	}

	if (acl_htable_enter(__fhandle_table, file_path, fs) == NULL)
		acl_msg_fatal("%s(%d): add fpath(%s) to htable error(%s)",
			myname, __LINE__, PATH(fs->fp), acl_last_serror());
	fs->nrefer++;

	UNLOCK_FS;

	/* 是否打开后自动加锁 */
	if ((oflags & ACL_FHANDLE_O_EXCL) != 0)
		acl_fhandle_lock(fs);

	return (fs);
}

/* 真正关闭并释放存储对象 */

static void __fhandle_close(ACL_FHANDLE *fs)
{
	const char *myname = "__fhandle_close";

	if (fs->nrefer != 0)
		acl_msg_fatal("%s: nrefer: %d != 0", myname, fs->nrefer);

	if (fs->on_close)
		fs->on_close(fs);

	acl_ring_detach(&fs->ring);
	(void) acl_htable_delete(__fhandle_table, PATH(fs->fp), NULL);

	__fhandle_free(fs);
}

void acl_fhandle_close(ACL_FHANDLE *fs, int delay_timeout)
{
	const char *myname = "acl_fhandle_close";
	time_t  now;
	ACL_RING *iter, *iter_next;
	ACL_RING_ITER ring_iter;

	LOCK_FS;

	fs->nrefer--;
	if (fs->nrefer == 0) {
		ACL_FHANDLE *fhandle_iter;

		if (delay_timeout <= 0) {
			__fhandle_close(fs);
			if ((fs->status & ACL_FHANDLE_S_MUTEX_ON) != 0)
				acl_fhandle_unlock(fs);
			UNLOCK_FS;
			return;
		}

		fs->when_free = time(NULL) + delay_timeout;  /* 默认的关闭时间截 */
		iter = &__fhandle_free_list;

		/* 数据链的存储方式为，由头 --> 尾，则数据由 旧 --> 新, 即: 时间值 小 --> 大 */
		
		/* 从尾部开始扫描，即扫描时间过期值由大向小的方向扫描所有对象，找到
		 * 过期时间值比当前对象的时间值小或相等的第一个对象
		 */
		acl_ring_foreach_reverse(ring_iter, &__fhandle_free_list) {
			fhandle_iter = acl_ring_to_appl(ring_iter.ptr, ACL_FHANDLE, ring);
			if (fs->when_free >= fhandle_iter->when_free) {
				iter = ring_iter.ptr;
				break;
			}
		}
		/* 放在比扫描对象后从而保持数据链的大小顺序不变 */
		acl_ring_append(iter, &fs->ring);

		/* 解锁 */
		if ((fs->status & ACL_FHANDLE_S_MUTEX_ON) != 0)
			acl_fhandle_unlock(fs);

		/* 由头部扫描的方法效率较低
		 * 从头部开始扫描，即扫描时间过期值由小向大的方向扫描所有对象，找到
		 * 过期时间值比当前对象的过期时间大或相等的第一个对象
		 * acl_foreach_ring(iter_next, &__fhandle_free_list) {
		 *	fhandle_iter = ACL_RING_TO_APPL(iter_next, ACL_FHANDLE, ring);
		 *	if (fs->when_free <= fhandle_iter->when_free) {
		 *		iter = iter_next;
		 *		break;
		 *	}
		 * }
		 * 将当前对象放在扫描对象的前面从而保持数据链的大小顺序不变
		 * acl_ring_prepend(iter, &fs->ring);
		 */
		
		if (acl_ring_size(&__fhandle_free_list) > __cache_max_size) {
			/* 弹出最老的对象进行释放 */
			iter = acl_ring_pop_head(&__fhandle_free_list);
			fs = ACL_RING_TO_APPL(iter, ACL_FHANDLE, ring);
			if (fs->nrefer != 0)
				acl_msg_fatal("%s: fpath: nrefer: %d != 0, list size: %d",
					myname, fs->nrefer, acl_ring_size(&__fhandle_free_list));
			if ((fs->status & ACL_FHANDLE_S_MUTEX_ON) != 0)
				acl_fhandle_unlock(fs);
			__fhandle_close(fs);
		}
	} else if ((fs->status & ACL_FHANDLE_S_MUTEX_ON) != 0)
		acl_fhandle_unlock(fs);

	(void) time(&now);

	/* 因为数据链中的对象的过期时间值由头 --> 尾是按由 小 --> 大的顺序存储的,
	 * 所以头部的数据应最先过期; 释放掉过期的延期关闭的缓存对象
	 */
        for (iter = acl_ring_succ(&__fhandle_free_list); iter != &__fhandle_free_list;) {
		fs = ACL_RING_TO_APPL(iter, ACL_FHANDLE, ring);
		if (fs->when_free > now)
			break;
		if (fs->nrefer > 0) {
			acl_msg_warn("%s: fs(%s)'s nrefer(%d) > 0, which in free list",
				myname, PATH(fs->fp), fs->nrefer);
			iter = acl_ring_succ(iter);
			continue;
		}
		iter_next = acl_ring_succ(iter);
		if (fs->nrefer == 0) {
			acl_debug(__debug_section, 2)
				("%s: fpath: %s, when_free: %ld, now: %ld",
				myname, PATH(fs->fp), fs->when_free, now);
			__fhandle_close(fs);
		}
		iter = iter_next;
	}

	UNLOCK_FS;
}

void acl_fhandle_lock(ACL_FHANDLE *fs)
{
	const char *myname = "acl_fhandle_lock";
#ifdef ACL_WINDOWS
	unsigned long tid = acl_pthread_self();
#else
	acl_pthread_t tid = acl_pthread_self();
#endif

	if ((fs->oflags & ACL_FHANDLE_O_MLOCK) == 0) {
		acl_msg_error("%s(%d): ACL_FHANDLE_O_MLOCK not set", myname, __LINE__);
		return;
	}
	/* 加线程锁，同时允许同一线程可以对同一锁加锁多次 */
	MUTEX_LOCK(&fs->mutex);
	fs->status |= ACL_FHANDLE_S_MUTEX_ON;
	fs->lock_mutex_tid = tid;

	/* 加进程文件锁以防止不同进程同时访问 */
	if ((fs->oflags & ACL_FHANDLE_O_FLOCK)
		&& (fs->status & ACL_FHANDLE_S_FLOCK_ON) == 0)
	{
		if (acl_myflock(ACL_VSTREAM_FILE(fs->fp),
			ACL_FLOCK_STYLE_FCNTL, ACL_FLOCK_OP_EXCLUSIVE) == -1)
		{
			acl_msg_fatal("%s: lock file(%s) error(%s)",
				myname, PATH(fs->fp), acl_last_serror());
		}
		fs->status |= ACL_FHANDLE_S_FLOCK_ON;
	}
}

void acl_fhandle_unlock(ACL_FHANDLE *fs)
{
	const char *myname = "acl_fhandle_unlock";

	/* sanity check */

	if ((fs->oflags & ACL_FHANDLE_O_MLOCK) == 0)
		return;

	if (fs->lock_mutex_tid != acl_pthread_self()) {
		acl_msg_warn("%s(%d): thread not locked mutex for %s",
			myname, __LINE__, PATH(fs->fp));
		return;
	} else if ((fs->status & ACL_FHANDLE_S_MUTEX_ON) == 0) {
		acl_msg_warn("%s(%d): thread not locked mutex for %s",
			myname, __LINE__, PATH(fs->fp));
		return;
	}

	/* xxx: 必须清除线程锁的所有者 */
#ifdef ACL_WINDOWS
	fs->lock_mutex_tid = (unsigned int) -1;
#else
	fs->lock_mutex_tid = (acl_pthread_t) -1;
#endif
	fs->status &= ~ACL_FHANDLE_S_MUTEX_ON;

	if ((fs->status & ACL_FHANDLE_S_FLOCK_ON) != 0) {
		fs->status &= ~ACL_FHANDLE_S_FLOCK_ON;

		if (acl_myflock(ACL_VSTREAM_FILE(fs->fp),
			ACL_FLOCK_STYLE_FCNTL, ACL_FLOCK_OP_NONE) == -1)
		{
			acl_msg_fatal("%s: unlock file(%s) error(%s)",
				myname, PATH(fs->fp), acl_last_serror());
		}
	}

	if (MUTEX_UNLOCK(&fs->mutex) != 0)
		acl_msg_fatal("%s: unlock mutex for %s error(%s)",
			myname, PATH(fs->fp), acl_last_serror());
}

void acl_fhandle_init(int cache_size, int debug_section, unsigned int flags)
{
	const char *myname = "acl_fhandle_init";
#ifdef	ACL_UNIX
	acl_pthread_mutexattr_t attr;
#endif

	if (__fhandle_table != NULL) {
		acl_msg_warn("%s(%d): __fhandle_table not null", myname, __LINE__);
		return;
	}

	__cache_max_size = cache_size > 0 ? cache_size : 100;
	__debug_section = debug_section;
#ifdef ACL_WINDOWS
	/* win32 下文件名及路径名是不区分大小写的，所以基于文件路径为键的哈希表
	 * 需要设置为自动转为小写键值的情况
	 */
	__fhandle_table = acl_htable_create(100, ACL_HTABLE_FLAG_KEY_LOWER);
#else
	__fhandle_table = acl_htable_create(100, 0);
#endif
	__flags = flags;
	acl_ring_init(&__fhandle_free_list);

#ifdef	ACL_UNIX
	pthread_mutexattr_init(&attr);
# if	defined(ACL_FREEBSD) || defined(ACL_SUNOS5) || defined(ACL_MACOSX)
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
# elif	defined(MINGW)
	pthread_mutex_init(&__fhandle_mutex, &attr);
# else
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
# endif
	pthread_mutex_init(&__fhandle_mutex, &attr);
#else
	acl_pthread_mutex_init(&__fhandle_mutex, NULL);
#endif
}

static void __fhandle_close2(ACL_FHANDLE *fs)
{
	const char *myname = "__fhandle_close2";

	if (fs->nrefer != 0)
		acl_msg_warn("%s: nrefer: %d != 0, name: %s",
			myname, fs->nrefer, ACL_FHANDLE_PATH(fs));

	if (fs->on_close)
		fs->on_close(fs);

	acl_ring_detach(&fs->ring);

	if ((fs->status & ACL_FHANDLE_S_MUTEX_ON) != 0) {
		acl_msg_warn("%s: fpath: %s, mutex not release yet",
			myname, PATH(fs->fp));
		MUTEX_UNLOCK(&fs->mutex);
	}
	fs->status &= ~ACL_FHANDLE_S_MUTEX_ON;

	__fhandle_free(fs);
}

void acl_fhandle_end()
{
	const char *myname = "acl_fhandle_end";

	acl_msg_info("%s(%d): close all ACL_FHANDLE now", myname, __LINE__);

	if (__fhandle_table == NULL) {
		acl_msg_warn("%s(%d): __fhandle_table null", myname, __LINE__);
		return;
	}
	acl_htable_free(__fhandle_table, (void (*)(void*)) __fhandle_close2);
	acl_pthread_mutex_destroy(&__fhandle_mutex);
	__fhandle_table = NULL;
}
