#ifndef	PTHREAD_INCLUDE_H
#define	PTHREAD_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
#include <time.h>

#define __thread __declspec(thread)

//#define	TLS_OUT_OF_INDEXES         0xffffffff
#define	PTHREAD_KEYS_MAX           1024
#define PTHREAD_ONCE_INIT          0
typedef unsigned long pthread_t;
typedef struct pthread_mutex_t pthread_mutex_t;
typedef struct pthread_mutexattr_t pthread_mutexattr_t;
typedef int pthread_key_t;
typedef int pthread_once_t;

struct pthread_mutex_t {
	HANDLE id;
	char   dynamic;
};

struct pthread_mutexattr_t {
	SECURITY_ATTRIBUTES attr;
};

int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));
int pthread_key_create(pthread_key_t *key_ptr, void (*destructor)(void*));

void *pthread_getspecific(pthread_key_t key);
int pthread_setspecific(pthread_key_t key, void *value);

int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *mattr);
int pthread_mutex_destroy(pthread_mutex_t *mutex);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);

#endif // _WIN32 || _WIN64

unsigned long __pthread_self(void);

#ifdef	__cplusplus
}
#endif

#endif
