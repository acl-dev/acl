#include "StdAfx.h"

#ifdef USE_TLS
#include <openssl/crypto.h>
#include "tls_private.h"

struct CRYPTO_dynlock_value {
	acl_pthread_mutex_t mutex;
};

static long *lock_count;
static acl_pthread_mutex_t *lock_cs;

static void threads_locking_fn(int mode, int type,
	const char *file acl_unused, int line acl_unused)
{
	if (mode & CRYPTO_LOCK) {
		acl_pthread_mutex_lock(&(lock_cs[type]));
		lock_count[type]++;
	} else {
		acl_pthread_mutex_unlock(&(lock_cs[type]));
	}
}

static unsigned long threads_thread_id_fn(void)
{
#ifdef	ACL_UNIX
	acl_pthread_t ret;
	ret = acl_pthread_self();
#elif defined(WIN32)
	unsigned long ret;
	ret = acl_pthread_self();
#else
#error "unknown OS"
#endif
	return((unsigned long) ret);
}

static struct CRYPTO_dynlock_value
*dynlock_create_fn(const char *file acl_unused, int line acl_unused)
{
	struct CRYPTO_dynlock_value *value;

	value = acl_mymalloc(sizeof(struct CRYPTO_dynlock_value));
	if (value == NULL)
		return (NULL);
	acl_pthread_mutex_init(&value->mutex, NULL);
	return (value);
}

static void dynlock_lock_fn(int mode, struct CRYPTO_dynlock_value *value,
	const char *file acl_unused, int line acl_unused)
{
	if (mode &CRYPTO_LOCK)
		acl_pthread_mutex_lock(&value->mutex);
	else
		acl_pthread_mutex_unlock(&value->mutex);
}

static void dynlock_destroy_fn(struct CRYPTO_dynlock_value *value,
	const char *file acl_unused, int line acl_unused)
{
	acl_pthread_mutex_destroy(&value->mutex);
	acl_myfree(value);
}

void tls_threads_init()
{
	int   i;

	lock_cs = OPENSSL_malloc(CRYPTO_num_locks() * sizeof(acl_pthread_mutex_t));
	lock_count = OPENSSL_malloc(CRYPTO_num_locks() * sizeof(long));

	/* Initialize OpenSSL locking callback */
	for (i = 0; i < CRYPTO_num_locks(); i++) {
		lock_count[i] = 0;
		acl_pthread_mutex_init(&(lock_cs[i]),NULL);
	}
	CRYPTO_set_id_callback(threads_thread_id_fn);
	CRYPTO_set_locking_callback(threads_locking_fn);

	/* Initialize OpenSSL dynamic locks callbacks */
	CRYPTO_set_dynlock_create_callback(dynlock_create_fn);
	CRYPTO_set_dynlock_lock_callback(dynlock_lock_fn);
	CRYPTO_set_dynlock_destroy_callback(dynlock_destroy_fn);
}

#endif
