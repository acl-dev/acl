
#ifndef	__INTERNAL_SEM_INCLUDE_H__
#define	__INTERNAL_SEM_INCLUDE_H__

#ifdef	__cplusplus
extern "C" {
#endif
#include "stdlib/acl_define.h"

#if defined(_WIN32) || defined(_WIN64)
#include "thread/acl_sem.h"

ACL_SEM *sem_create2(const char *pathname, unsigned int initial_value);
ACL_SEM *sem_create(unsigned int initial_value);
void sem_destroy(ACL_SEM *sem);
int sem_wait_timeout(ACL_SEM *sem, unsigned int timeout);
int sem_try_wait(ACL_SEM *sem);
int sem_wait(ACL_SEM *sem);
unsigned int sem_value(ACL_SEM *sem);
int sem_post(ACL_SEM *sem);

#endif

#ifdef	__cplusplus
}
#endif

#endif
