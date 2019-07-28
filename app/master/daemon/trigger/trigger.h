#ifndef __TRIGGER_INCLUDED_H__
#define __TRIGGER_INCLUDED_H__

#ifdef	__cplusplus
extern "C" {
#endif

#ifdef SUNOS5
#define ACL_LOCAL_TRIGGER   acl_stream_trigger
#else
#define ACL_LOCAL_TRIGGER   acl_unix_trigger
#endif

 /*
  * External interface.
  */
extern int acl_unix_trigger(ACL_EVENT *, const char *, const char *, int, int);
extern int acl_inet_trigger(ACL_EVENT *, const char *, const char *, int, int);
extern int acl_fifo_trigger(ACL_EVENT *, const char *, const char *, int, int);
#ifdef SUNOS5
extern int acl_stream_trigger(ACL_EVENT *, const char *, const char *, int, int);
#endif

#ifdef	__cplusplus
}
#endif

#endif

