
#ifndef	__ACL_MASTER_SERVER_API_INCLUDE_H_
#define	__ACL_MASTER_SERVER_API_INCLUDE_H_

#ifdef	__cplusplus
extern "C" {
#endif

#include "stdlib/acl_define.h"
#ifdef ACL_UNIX

 /*
  * Utility library.
  */
#include "stdlib/acl_vstream.h"
#include "ioctl/acl_ioctl.h"
#include "aio/acl_aio.h"
#include "event/acl_events.h"

 /*
  * External interface. Tables are defined in mail_conf.h.
  */
#define ACL_MASTER_SERVER_INT_TABLE		1
#define ACL_MASTER_SERVER_STR_TABLE		2
#define ACL_MASTER_SERVER_BOOL_TABLE		3
#define ACL_MASTER_SERVER_TIME_TABLE		4
#define ACL_MASTER_SERVER_RAW_TABLE		5
#define ACL_MASTER_SERVER_INT64_TABLE		6

#define	ACL_MASTER_SERVER_PRE_INIT		10
#define ACL_MASTER_SERVER_POST_INIT		11
#define ACL_MASTER_SERVER_LOOP			12
#define ACL_MASTER_SERVER_EXIT			13
#define ACL_MASTER_SERVER_PRE_ACCEPT		14
#define ACL_MASTER_SERVER_SOLITARY		15
#define ACL_MASTER_SERVER_UNLIMITED		16
#define ACL_MASTER_SERVER_PRE_DISCONN		17
#define ACL_MASTER_SERVER_PRIVILEGED		18
#define	ACL_MASTER_SERVER_ON_ACCEPT		19

#define ACL_MASTER_SERVER_IN_FLOW_DELAY		20

#define	ACL_MASTER_SERVER_RW_TIMER		30

#define	ACL_MASTER_SERVER_THREAD_INIT		50
#define	ACL_MASTER_SERVER_THREAD_INIT_CTX	51
#define	ACL_MASTER_SERVER_THREAD_EXIT		52
#define	ACL_MASTER_SERVER_THREAD_EXIT_CTX	53

typedef void (*ACL_MASTER_SERVER_INIT_FN) (char *, char **);
typedef int  (*ACL_MASTER_SERVER_LOOP_FN) (char *, char **);
typedef void (*ACL_MASTER_SERVER_EXIT_FN) (char *, char **);
typedef void (*ACL_MASTER_SERVER_ACCEPT_FN) (char *, char **);
typedef void (*ACL_MASTER_SERVER_DISCONN_FN) (ACL_VSTREAM *, char *, char **);
typedef int  (*ACL_MASTER_SERVER_ON_ACCEPT_FN) (ACL_VSTREAM *);

/* see acl_ioctl.h */
typedef ACL_IOCTL_THREAD_INIT_FN ACL_MASTER_SERVER_THREAD_INIT_FN;
typedef ACL_IOCTL_THREAD_EXIT_FN ACL_MASTER_SERVER_THREAD_EXIT_FN;

/* add by zsx for rw timeout, 2005.9.25*/
typedef void (*ACL_MASTER_SERVER_RW_TIMER_FN) (ACL_VSTREAM *);

 /*
  * acl_single_server.c
  */
typedef void (*ACL_SINGLE_SERVER_FN) (ACL_VSTREAM *, char *, char **);
extern void acl_single_server_main(int, char **, ACL_SINGLE_SERVER_FN, ...);
extern ACL_EVENT *acl_single_server_event(void);
extern ACL_VSTREAM **acl_single_server_sstreams(void);

 /*
  * acl_multi_server.c
  */
typedef void (*ACL_MULTI_SERVER_FN) (ACL_VSTREAM *, char *, char **);
/* add by zsx for rw timeout, 2005.9.25*/
void acl_multi_server_request_rw_timer(ACL_VSTREAM *);
void acl_multi_server_cancel_rw_timer(ACL_VSTREAM *);
/* end add, 2005.9.25 */
extern void acl_multi_server_main(int, char **, ACL_MULTI_SERVER_FN,...);
extern void acl_multi_server_disconnect(ACL_VSTREAM *);
extern int acl_multi_server_drain(void);
extern ACL_EVENT *acl_multi_server_event(void);
extern void acl_multi_server_enable_read(ACL_VSTREAM *stream);

 /*
  * acl_ioctl_server.c
  */
typedef void (*ACL_IOCTL_SERVER_FN) (ACL_IOCTL *, ACL_VSTREAM *, char *, char **);
extern void acl_ioctl_server_request_timer(ACL_EVENT_NOTIFY_TIME timer_fn,
	void *arg, int delay);
extern void acl_ioctl_server_cancel_timer(ACL_EVENT_NOTIFY_TIME timer_fn,
	void *arg);
extern void acl_ioctl_server_main(int, char **, ACL_IOCTL_SERVER_FN,...);
extern ACL_IOCTL *acl_ioctl_server_handle(void);
extern ACL_EVENT *acl_ioctl_server_event(void);
extern ACL_VSTREAM **acl_ioctl_server_streams(void);
extern void acl_ioctl_server_enable_read(ACL_IOCTL *h_ioctl, ACL_VSTREAM *stream,
	int timeout, ACL_IOCTL_NOTIFY_FN notify_fn, void *context);

 /*
  * acl_threads_server.c
  */
ACL_EVENT *acl_threads_server_event(void);
acl_pthread_pool_t *acl_threads_server_threads(void);
ACL_VSTREAM **acl_threads_server_streams(void);
void acl_threads_server_main(int argc, char **argv,
	int (*service)(ACL_VSTREAM*, void*), void *service_ctx, int name, ...);

 /*
  * acl_aio_server.c
  */
typedef void (*ACL_AIO_SERVER_FN) (ACL_SOCKET, char *, char **);
extern void acl_aio_server_request_timer(ACL_EVENT_NOTIFY_TIME timer_fn,
	void *arg, int delay);
extern void acl_aio_server_cancel_timer(ACL_EVENT_NOTIFY_TIME timer_fn,
	void *arg);
extern void acl_aio_server_request_rw_timer(ACL_ASTREAM *);
extern void acl_aio_server_cancel_rw_timer(ACL_ASTREAM *);
extern void acl_aio_server_main(int, char **, ACL_AIO_SERVER_FN, ...);
extern ACL_AIO *acl_aio_server_handle(void);
extern ACL_EVENT *acl_aio_server_event(void);
extern int acl_aio_server_read(ACL_ASTREAM *astream, int timeout,
	ACL_AIO_READ_FN notify_fn, void *context);
extern int acl_aio_server_readn(ACL_ASTREAM *astream, int count, int timeout,
	ACL_AIO_READ_FN notify_fn, void *context);
extern int acl_aio_server_gets(ACL_ASTREAM *astream, int timeout,
	ACL_AIO_READ_FN notify_fn, void *context);
extern int acl_aio_server_gets_nonl(ACL_ASTREAM *astream, int timeout,
	ACL_AIO_READ_FN notify_fn, void *context);
extern int acl_aio_server_writen(ACL_ASTREAM *astream,
	ACL_AIO_WRITE_FN notify_fn, void *context,
	const char *data, int dlen);
extern int acl_aio_server_vfprintf(ACL_ASTREAM *astream,
	ACL_AIO_WRITE_FN notify_fn, void *context,
	const char *fmt, va_list ap);
extern int acl_aio_server_fprintf(ACL_ASTREAM *astream,
	ACL_AIO_WRITE_FN notify_fn, void *context, const char *fmt, ...);
extern int acl_aio_server_connect(const char *saddr, int timeout,
	ACL_AIO_CONNECT_FN connect_fn, void *context);
extern ACL_ASTREAM **acl_aio_server_streams(void);
extern void acl_aio_server_on_close(ACL_ASTREAM *stream);

 /*
  * acl_udp_server.c
  */
typedef void (*ACL_UDP_SERVER_FN) (ACL_VSTREAM *, char *, char **);
extern void acl_udp_server_request_timer(ACL_EVENT_NOTIFY_TIME timer_fn,
	void *arg, acl_int64 delay, int keep);
extern void acl_udp_server_cancel_timer(ACL_EVENT_NOTIFY_TIME timer_fn,
	void *arg);
extern void acl_udp_server_main(int, char **, ACL_UDP_SERVER_FN, ...);
extern ACL_EVENT *acl_udp_server_event(void);
extern ACL_VSTREAM **acl_udp_server_streams(void);

 /*
  * acl_listener_server.c
  */
typedef void (*ACL_LISTEN_SERVER_FN) (ACL_VSTREAM *, char *, char **);
/* add by zsx for rw timeout, 2005.9.25*/
void acl_listener_server_request_rw_timer(ACL_VSTREAM *);
void acl_listener_server_cancel_rw_timer(ACL_VSTREAM *);
/* end add, 2005.9.25 */
extern void acl_listener_server_main(int, char **, ACL_MULTI_SERVER_FN,...);
extern void acl_listener_server_disconnect(ACL_VSTREAM *);
extern int acl_listener_server_drain(void);

 /*
  * acl_trigger_server.c
  */
typedef void (*ACL_TRIGGER_SERVER_FN) (char *, int, char *, char **);
extern void acl_trigger_server_main(int, char **, ACL_TRIGGER_SERVER_FN, ...);
extern ACL_EVENT *acl_trigger_server_event(void);

/*
 * acl_master_log.c
 */
extern void acl_master_log_open(const char *procname);

#define ACL_TRIGGER_BUF_SIZE	1024

#endif /* ACL_UNIX */

#ifdef	__cplusplus
}
#endif

#endif

