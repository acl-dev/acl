#ifndef ACL_ATOMIC_INCLUDE_H
#define ACL_ATOMIC_INCLUDE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "acl_define.h"

typedef struct ACL_ATOMIC ACL_ATOMIC;

ACL_API ACL_ATOMIC *acl_atomic_new(void);
ACL_API void  acl_atomic_free(ACL_ATOMIC *self);
ACL_API void  acl_atomic_set(ACL_ATOMIC *self, void *value);
ACL_API void *acl_atomic_cas(ACL_ATOMIC *self, void *cmp, void *value);
ACL_API void *acl_atomic_xchg(ACL_ATOMIC *self, void *value);
ACL_API void acl_atomic_int64_set(ACL_ATOMIC *self, long long n);
ACL_API long long acl_atomic_int64_fetch_add(ACL_ATOMIC *self, long long n);
ACL_API long long acl_atomic_int64_add_fetch(ACL_ATOMIC *self, long long n);

typedef struct ACL_ATOMIC_CLOCK ACL_ATOMIC_CLOCK;

ACL_API ACL_ATOMIC_CLOCK *acl_atomic_clock_alloc(void);
ACL_API void acl_atomic_clock_free(ACL_ATOMIC_CLOCK *clk);
ACL_API long long acl_atomic_clock_count_add(ACL_ATOMIC_CLOCK *clk, int n);
ACL_API long long acl_atomic_clock_users_add(ACL_ATOMIC_CLOCK *clk, int n);
ACL_API void acl_atomic_clock_users_count_inc(ACL_ATOMIC_CLOCK *clk);
ACL_API long long acl_atomic_clock_count(ACL_ATOMIC_CLOCK *clk);
ACL_API long long acl_atomic_clock_atime(ACL_ATOMIC_CLOCK *clk);
ACL_API long long acl_atomic_clock_users(ACL_ATOMIC_CLOCK *clk);

#ifdef __cplusplus
}
#endif

#endif
