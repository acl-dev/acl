#ifndef MASTER_VARS_INCLUDE_H
#define MASTER_VARS_INCLUDE_H

extern acl_pthread_pool_t *acl_var_master_thread_pool;

void acl_master_vars_init(int buf_size, int rw_timeout);
void acl_master_vars_end(void);

#endif
