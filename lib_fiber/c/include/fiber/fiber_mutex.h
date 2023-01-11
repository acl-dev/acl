#ifndef FIBER_MUTEX_INCLUDE_H
#define	FIBER_MUTEX_INCLUDE_H

#include "fiber_define.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ACL_FIBER_MUTEX ACL_FIBER_MUTEX;

#define	FIBER_MUTEX_F_LOCK_TRY		(1 << 0)
#define	FIBER_MUTEX_F_SWITCH_FIRST	(1 << 1)

FIBER_API ACL_FIBER_MUTEX *acl_fiber_mutex_create(unsigned flag);
FIBER_API void acl_fiber_mutex_free(ACL_FIBER_MUTEX *mutex);
FIBER_API int acl_fiber_mutex_lock(ACL_FIBER_MUTEX *mutex);
FIBER_API int acl_fiber_mutex_trylock(ACL_FIBER_MUTEX *mutex);
FIBER_API int acl_fiber_mutex_unlock(ACL_FIBER_MUTEX *mutex);

typedef struct ACL_FIBER_MUTEX_STAT {
	ACL_FIBER *fiber;		// The fiber with the stat object.
	ACL_FIBER_MUTEX *waiting;	// The mutex been waited by the fiber.
	ACL_FIBER_MUTEX **holding;	// The mutexes held by the fiber.
	size_t count;			// The holding's count.
} ACL_FIBER_MUTEX_STAT;

typedef struct ACL_FIBER_MUTEX_STATS {
	ACL_FIBER_MUTEX_STAT *stats;	// The array holding the stats above.
	size_t count;			// The stats array's size.
} ACL_FIBER_MUTEX_STATS;


FIBER_API void acl_fiber_mutex_profile(void);
FIBER_API ACL_FIBER_MUTEX_STATS *acl_fiber_mutex_deadlock(void);
FIBER_API void acl_fiber_mutex_stats_free(ACL_FIBER_MUTEX_STATS *stats);
FIBER_API void acl_fiber_mutex_stats_show(const ACL_FIBER_MUTEX_STATS *stats);

#ifdef __cplusplus
}
#endif

#endif
