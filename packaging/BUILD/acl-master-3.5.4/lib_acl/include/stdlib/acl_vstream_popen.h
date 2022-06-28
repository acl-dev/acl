#ifndef	ACL_VSTREAM_POPEN_INCLUDE_H
#define	ACL_VSTREAM_POPEN_INCLUDE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "acl_define.h"
#include "acl_vstream.h"

#ifdef	ACL_UNIX
#define	DUP2	dup2
#endif

ACL_API ACL_VSTREAM *acl_vstream_popen(int,...);
ACL_API int acl_vstream_pclose(ACL_VSTREAM *);

#define acl_vstream_ispipe(vp)	((vp)->pid != 0)

#define ACL_VSTREAM_POPEN_END		0	/* terminator */
#define ACL_VSTREAM_POPEN_COMMAND	1	/* command is string */
#define ACL_VSTREAM_POPEN_ARGV		2	/* command is array */
#define ACL_VSTREAM_POPEN_UID		3	/* privileges */
#define ACL_VSTREAM_POPEN_GID		4	/* privileges */
#define ACL_VSTREAM_POPEN_ENV		5	/* extra environment */
#define ACL_VSTREAM_POPEN_SHELL		6	/* alternative shell */
#define ACL_VSTREAM_POPEN_WAITPID_FN	7	/* child catcher, waitpid() compat. */
#define ACL_VSTREAM_POPEN_EXPORT	8	/* exportable environment */

#ifdef	__cplusplus
}
#endif

#endif
