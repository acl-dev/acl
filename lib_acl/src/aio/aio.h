#ifndef __INTERNAL_AIO_INCLUDE_H__
#define __INTERNAL_AIO_INCLUDE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#include "stdlib/acl_vstream.h"
#include "stdlib/acl_vstring.h"
#include "stdlib/acl_array.h"
#include "net/acl_dns.h"
#include "aio/acl_aio.h"

struct ACL_AIO {
	ACL_EVENT *event;
	int   delay_sec;
	int   delay_usec;
	int   keep_read;
	int   rbuf_size;
	int   event_mode;
	ACL_ARRAY *dead_streams;
#if defined(_WIN32) || defined(_WIN64)
	int   timer_active;
	unsigned int tid;
#endif
	ACL_DNS *dns;
};

typedef struct AIO_READ_HOOK {
	ACL_AIO_READ_FN callback;
	void *ctx;
	char  disable;
} AIO_READ_HOOK;

typedef struct AIO_WRITE_HOOK {
	ACL_AIO_WRITE_FN callback;
	void *ctx;
	char  disable;
} AIO_WRITE_HOOK;

typedef struct AIO_CLOSE_HOOK {
	ACL_AIO_CLOSE_FN callback;
	void *ctx;
	char  disable;
} AIO_CLOSE_HOOK;

typedef struct AIO_TIMEO_HOOK {
	ACL_AIO_TIMEO_FN callback;
	void *ctx;
	char  disable;
} AIO_TIMEO_HOOK;

typedef struct AIO_CONNECT_HOOK {
	ACL_AIO_CONNECT_FN callback;
	void *ctx;
	char  disable;
} AIO_CONNECT_HOOK;

#define __AIO_NESTED_MAX	10
#define	__default_line_length	4096

/* in aio_callback.c */
int aio_timeout_callback(ACL_ASTREAM *astream);
void aio_close_callback(ACL_ASTREAM *astream);

/* in acl_aio_stream.c */
void aio_delay_check(ACL_AIO *aio);

#ifdef __cplusplus
}
#endif

#endif

