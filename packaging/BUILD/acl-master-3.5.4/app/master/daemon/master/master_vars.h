#ifndef __MASTER_VARS_INCLUDE_H__
#define __MASTER_VARS_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

extern acl_pthread_pool_t *acl_var_master_thread_pool;

void acl_master_vars_init(int buf_size, int rw_timeout);
void acl_master_vars_end(void);

#ifdef __cplusplus
}
#endif

#endif
