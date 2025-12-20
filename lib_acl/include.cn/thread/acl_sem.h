#ifndef	ACL_SEM_INCLUDE_H
#define	ACL_SEM_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif
#include "../stdlib/acl_define.h"

#if defined(_WIN32) || defined(_WIN64)

typedef struct ACL_SEM ACL_SEM;
struct ACL_SEM {
	HANDLE id;
	unsigned int volatile count;
};

ACL_API ACL_SEM *acl_sem_create2(const char *pathname,
		unsigned int initial_value);
ACL_API ACL_SEM *acl_sem_create(unsigned int initial_value);
ACL_API void acl_sem_destroy(ACL_SEM *sem);
ACL_API int acl_sem_wait_timeout(ACL_SEM *sem, unsigned int timeout);
ACL_API int acl_sem_try_wait(ACL_SEM *sem);
ACL_API int acl_sem_wait(ACL_SEM *sem);
ACL_API unsigned int acl_sem_value(ACL_SEM *sem);
ACL_API int acl_sem_post(ACL_SEM *sem);

#endif

#ifdef	__cplusplus
}
#endif

#endif
