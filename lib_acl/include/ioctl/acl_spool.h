#ifndef	ACL_SPOOL_INCLUDE_H
#define	ACL_SPOOL_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_ioctl.h"

/* 为了保持向后兼容 */

#define	ACL_SPOOL			ACL_IOCTL
#define	ACL_SPOOL_NOTIFY_FN		ACL_IOCTL_NOTIFY_FN
#define	ACL_SPOOL_TIMER_FN		ACL_IOCTL_TIMER_FN
#define	ACL_SPOOL_WORKER_FN		ACL_IOCTL_WORKER_FN
#define	acl_spool_create		acl_ioctl_create
#define	acl_spool_create_ex		acl_ioctl_create_ex
#define	acl_spool_ctl			acl_ioctl_ctl
#define	ACL_SPOOL_CTL_END		ACL_IOCTL_CTL_END
#define	ACL_SPOOL_CTL_THREAD_MAX	ACL_IOCTL_CTL_THREAD_MAX
#define	ACL_SPOOL_CTL_THREAD_IDLE	ACL_IOCTL_CTL_THREAD_IDLE
#define	ACL_SPOOL_CTL_DELAY_SEC		ACL_IOCTL_CTL_DELAY_SEC
#define	ACL_SPOOL_CTL_DELAY_USEC	ACL_IOCTL_CTL_DELAY_USEC
#define	acl_spool_free			acl_ioctl_free
#define	acl_spool_start			acl_ioctl_start
#define	acl_spool_loop			acl_ioctl_loop
#define	acl_spool_disable_readwrite	acl_ioctl_disable_readwrite
#define	acl_spool_isset			acl_ioctl_isset
#define	acl_spool_isrset		acl_ioctl_isrset
#define	acl_spool_iswset		acl_ioctl_iswset
#define	acl_spool_enable_read		acl_ioctl_enable_read
#define	acl_spool_enable_write		acl_ioctl_enable_write
#define	acl_spool_enable_connect	acl_ioctl_enable_connect
#define	acl_spool_enable_listen		acl_ioctl_enable_listen
#define	acl_spool_connect		acl_ioctl_connect
#define	acl_spool_listen		acl_ioctl_listen
#define	acl_spool_listen_ex		acl_ioctl_listen_ex
#define	acl_spool_accept		acl_ioctl_accept
#define	acl_spool_request_timer		acl_ioctl_request_timer
#define	acl_spool_cancel_timer		acl_ioctl_cancel_timer
#define	acl_spool_add			acl_ioctl_add
#define	acl_spool_nworker		acl_ioctl_nworker

#ifdef	__cplusplus
}
#endif

#endif
