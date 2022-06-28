#ifndef	ACL_FHANDLE_INCLUDE_H
#define	ACL_FHANDLE_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_vstream.h"
#include "acl_vstring.h"
#include "acl_ring.h"
#include "../thread/acl_thread.h"
#include <time.h>

/**
 * 通用的存储文件句柄对象类型定义
 */

typedef struct ACL_FHANDLE	ACL_FHANDLE;

struct ACL_FHANDLE {
	ACL_VSTREAM *fp;			/**< 存储文件句柄 */
	acl_int64 fsize;			/**< 存储文件大小 */
	int   nrefer;				/**< 该存储句柄被引用的计数值 */
	acl_pthread_mutex_t mutex;		/**< 线程锁 */
#if defined(_WIN32) || defined(_WIN64)
	unsigned long tid;			/**< 打开该存储的线程号 */
	unsigned long lock_mutex_tid;		/**< 加线程锁的线程号 */
#else
	acl_pthread_t tid;			/**< 打开该存储的线程号 */
	acl_pthread_t lock_mutex_tid;		/**< 加线程锁的线程号 */
#endif
	unsigned int oflags;			/**< 打开时的标志位 */
#define	ACL_FHANDLE_O_FLOCK	(1 << 0)	/**< 使用文件锁 */
#define	ACL_FHANDLE_O_MLOCK	(1 << 1)	/**< 使用线程锁 */
#define	ACL_FHANDLE_O_MKDIR	(1 << 2)	/**< 是否自动检查并创建不存在的目录 */
#define	ACL_FHANDLE_O_NOATIME	(1 << 3)	/**< 打开文件时添加 O_NOATIME 标志位 */
#define	ACL_FHANDLE_O_DIRECT	(1 << 4)	/**< 打开文件时添加 O_DIRECT 标志位 */
#define	ACL_FHANDLE_O_SYNC	(1 << 5)	/**< 打开文件时添加 O_SYNC 标志位 */
#define	ACL_FHANDLE_O_EXCL	(1 << 6)	/**< 打开文件时是否是自动加锁 */

	unsigned int status;			/**< 该存储文件句柄的状态 */
#define	ACL_FHANDLE_S_FLOCK_ON	(1 << 0)	/**< 该存储句柄已经加文件锁 */
#define	ACL_FHANDLE_S_MUTEX_ON	(1 << 1)	/**< 该存储句柄已经加线程锁 */

	time_t  when_free;			/**< 在延迟关闭缓存队列中存活的时间截 */
	ACL_RING ring;				/**< 缓存数据结点 */
	size_t size;				/**< 该 ACL_FHANDLE 对象的实际大小 >= sizeof(ACL_FHANDLE) */
	void (*on_close)(ACL_FHANDLE*);		/**< 当该文件缓存句柄真正关闭时的回调函数，可以为空 */
};

#define	ACL_FHANDLE_PATH(x)	(ACL_VSTREAM_PATH((x)->fp))

/**
 * 初始化文件句柄操作，该函数须在程序运行初始化时被调用且只能被调用一次
 * @param cache_size {int} 内部被打开文件句柄的最大个数
 * @param debug_section {int} 调试级别
 * @param flags {unsigned int}
 */
void acl_fhandle_init(int cache_size, int debug_section, unsigned int flags);
#define	ACL_FHANDLE_F_LOCK	(1 << 0)

/**
 * 当程序退出时需要调用此函数来释放系统资源
 */
void acl_fhandle_end(void);

/**
 * 打开一个文件
 * @param size {size_t} 分配结构 FS_HANDDLE 需要的空间大小
 * @param oflags {unsigned int} 打开文件句柄时的标志位, ACL_FHANDLE_O_XXX
 * @param file_path {const char*} 文件名(包含路径)
 * @param on_open {int (*)(ACL_FHANDLE*, void*)} 如果不为空，
 *  则当文件句柄被成功打开后便调用此函数
 * @param open_arg {void *} on_open 的回调参数之一
 * @param on_close {void (*)(ACL_FHANDLE*)} 如果不为空，
 *  则当文件句柄被正直关闭时便调用此函数
 */
ACL_FHANDLE *acl_fhandle_open(size_t size, unsigned int oflags,
	const char *file_path,
	int (*on_open)(ACL_FHANDLE*, void*), void *open_arg,
	void (*on_close)(ACL_FHANDLE*));

/**
 * 关闭一个文件句柄
 * @param fs {ACL_FHANDLE*}
 * @param delay_timeout {int} 如果 > 0, 则延迟该时间后才真正关闭,
 *  否则，其引用计数为 0 则立即关闭
 */
void acl_fhandle_close(ACL_FHANDLE *fs, int delay_timeout);

/**
 * 对一个文件句柄加锁(先加线程锁后加文件锁)
 * @param fs {ACL_FHANDLE*}
 */
void acl_fhandle_lock(ACL_FHANDLE *fs);

/**
 * 对一个文件句柄解锁(先解文件锁再解线程锁)
 * @param fs {ACL_FHANDLE*}
 */
void acl_fhandle_unlock(ACL_FHANDLE *fs);

#ifdef	__cplusplus
}
#endif

#endif
