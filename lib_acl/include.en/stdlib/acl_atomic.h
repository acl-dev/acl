#ifndef ACL_ATOMIC_INCLUDE_H
#define ACL_ATOMIC_INCLUDE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "acl_define.h"

typedef struct ACL_ATOMIC ACL_ATOMIC;

/**
 * Create atomic object.
 * @return {ACL_ATOMIC*} Newly created object
 */
ACL_API ACL_ATOMIC *acl_atomic_new(void);

/**
 * Free atomic object.
 * @param self {ACL_ATOMIC*} Atomic object
 */
ACL_API void acl_atomic_free(ACL_ATOMIC *self);

/**
 * Bind specified object to atomic object, to perform atomic
 * operations on this object.
 * @param self {ACL_ATOMIC*} Atomic object
 * @param value {void*} Bound object, perform atomic
 *  operations on this object through atomic object
 */
ACL_API void acl_atomic_set(ACL_ATOMIC *self, void *value);

/**
 * Compare and swap operation. If object bound to atomic
 * object equals comparison object, replace with new object.
 * Return previously bound object.
 * @param self {ACL_ATOMIC*} Atomic object
 * @param cmp {void*} Comparison object pointer
 * @param value {void*} If object bound to atomic object
 *  equals comparison object, will bind this object to
 *  atomic object
 * @return {void*} Previously bound object to atomic object
 */
ACL_API void *acl_atomic_cas(ACL_ATOMIC *self, void *cmp, void *value);

/**
 * Exchange new object with atomic object binding, return
 * previously bound object.
 * @param self {ACL_ATOMIC*} Atomic object
 * @param value {void*} New object to bind
 * @return {void*} Previously bound object
 */
ACL_API void *acl_atomic_xchg(ACL_ATOMIC *self, void *value);

/**
 * If object bound via acl_atomic_set is integer value, can
 * call this function to set bound object to integer value.
 * @param self {ACL_ATOMIC*} Atomic object
 * @param n {long long} Object bound to atomic object will
 *  be set to this value
 */
ACL_API void acl_atomic_int64_set(ACL_ATOMIC *self, long long n);

/**
 * Fetch current value, store it, then add specified value to stored value.
 * @param self {ACL_ATOMIC*} Atomic object
 * @param n {long long} Added value
 * @return {long long} Previously stored value before addition
 */
ACL_API long long acl_atomic_int64_fetch_add(ACL_ATOMIC *self, long long n);

/**
 * Add specified value to stored value, return result value.
 * @param self {ACL_ATOMIC*} Atomic object
 * @param n {long long} Added value
 * @return {long long} Result value after addition
 */
ACL_API long long acl_atomic_int64_add_fetch(ACL_ATOMIC *self, long long n);

/**
 * Compare and swap operation. If value stored in atomic
 * object equals comparison value, replace with new value
 * and return previously stored value.
 * @param self {ACL_ATOMIC*} Atomic object
 * @param cmp {long long} Comparison value
 * @param n {long long} If value stored in atomic object
 *  equals comparison value, will set atomic object to this
 *  value
 * @return {long long} Previously stored value in atomic object
 */
ACL_API long long acl_atomic_int64_cas(ACL_ATOMIC *self, long long cmp, long long n);

/****************************************************************************/

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
