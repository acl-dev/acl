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
 * Generic file handle cache object type definition.
 */

typedef struct ACL_FHANDLE	ACL_FHANDLE;

struct ACL_FHANDLE {
	ACL_VSTREAM *fp;			/**< Cached file handle */
	acl_int64 fsize;			/**< Cached file size */
	int   nrefer;				/**< Reference count value for this cached handle */
	acl_pthread_mutex_t mutex;		/**< Thread lock */
#if defined(_WIN32) || defined(_WIN64)
	unsigned long tid;			/**< Thread ID that opened this cached handle */
	unsigned long lock_mutex_tid;		/**< Thread ID that locked mutex */
#else
	acl_pthread_t tid;			/**< Thread ID that opened this cached handle */
	acl_pthread_t lock_mutex_tid;		/**< Thread ID that locked mutex */
#endif
	unsigned int oflags;			/**< Open flag bits */
#define	ACL_FHANDLE_O_FLOCK	(1 << 0)	/**< Use file lock */
#define	ACL_FHANDLE_O_MLOCK	(1 << 1)	/**< Use thread lock */
#define	ACL_FHANDLE_O_MKDIR	(1 << 2)	/**< Whether to automatically
						 *  check and create
						 *  non-existent directories */
#define	ACL_FHANDLE_O_NOATIME	(1 << 3)	/**< When opening file, add
						 *  O_NOATIME flag bit */
#define	ACL_FHANDLE_O_DIRECT	(1 << 4)	/**< When opening file, add
						 *  O_DIRECT flag bit */
#define	ACL_FHANDLE_O_SYNC	(1 << 5)	/**< When opening file, add
						 *  O_SYNC flag bit */
#define	ACL_FHANDLE_O_EXCL	(1 << 6)	/**< When opening file,
						 *  whether to automatically
						 *  create */

	unsigned int status;			/**< This cached file handle's status */
#define	ACL_FHANDLE_S_FLOCK_ON	(1 << 0)	/**< This cached handle has
						 *  file lock */
#define	ACL_FHANDLE_S_MUTEX_ON	(1 << 1)	/**< This cached handle has
						 *  thread lock */

	time_t  when_free;			/**< Time when delayed close
						 *  or delayed write occurs */
	ACL_RING ring;				/**< Internal ring node */
	size_t size;				/**< Actual size of
						 *  ACL_FHANDLE object >=
						 *  sizeof(ACL_FHANDLE) */
	void (*on_close)(ACL_FHANDLE*);		/**< Callback function when
						 *  file handle is about to
						 *  be closed, can be NULL */
};

#define	ACL_FHANDLE_PATH(x)	(ACL_VSTREAM_PATH((x)->fp))

/**
 * Initialize file handle cache. This function should be called during program
 * initialization, can only be called once.
 * @param cache_size {int} Internal cached file handle count
 * @param debug_section {int} Debug section
 * @param flags {unsigned int}
 */
void acl_fhandle_init(int cache_size, int debug_section, unsigned int flags);
#define	ACL_FHANDLE_F_LOCK	(1 << 0)

/**
 * When program exits, need to call this function to free system resources.
 */
void acl_fhandle_end(void);

/**
 * Open a file.
 * @param size {size_t} Required space size for structure FS_HANDDLE
 * @param oflags {unsigned int} Flag bits when opening file, ACL_FHANDLE_O_XXX
 * @param file_path {const char*} File name (can include path)
 * @param on_open {int (*)(ACL_FHANDLE*, void*)} If not NULL,
 *  after file handle is successfully opened, call this function
 * @param open_arg {void *} One of on_open's callback parameters
 * @param on_close {void (*)(ACL_FHANDLE*)} If not NULL,
 *  when file handle is about to be closed, call this function
 */
ACL_FHANDLE *acl_fhandle_open(size_t size, unsigned int oflags,
	const char *file_path,
	int (*on_open)(ACL_FHANDLE*, void*), void *open_arg,
	void (*on_close)(ACL_FHANDLE*));

/**
 * Close a file handle.
 * @param fs {ACL_FHANDLE*}
 * @param delay_timeout {int} If > 0, delay closing for this timeout duration,
 *  otherwise close immediately when reference count is 0
 */
void acl_fhandle_close(ACL_FHANDLE *fs, int delay_timeout);

/**
 * Lock a file handle (equivalent to thread lock and file lock).
 * @param fs {ACL_FHANDLE*}
 */
void acl_fhandle_lock(ACL_FHANDLE *fs);

/**
 * Unlock a file handle (first unlock file lock, then unlock thread lock).
 * @param fs {ACL_FHANDLE*}
 */
void acl_fhandle_unlock(ACL_FHANDLE *fs);

#ifdef	__cplusplus
}
#endif

#endif
